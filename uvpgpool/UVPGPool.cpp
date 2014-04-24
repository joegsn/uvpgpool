/*
Copyright (c) 2014, Joseph Love
All rights reserved.
 
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//
//  uv_postgresql.cpp
//  UVPGPool
//
//  Created by Joseph Love on 3/14/14.
//  Copyright (c) 2014 Gameowls. All rights reserved.
//

#include "UVPGPool.h"
#include <assert.h>
#include <atomic>

uint8_t ConnStatus::cs_invalid = 0;
uint8_t ConnStatus::cs_disconnecting = 1;
uint8_t ConnStatus::cs_connecting = 2;
uint8_t ConnStatus::cs_available = 3;
uint8_t ConnStatus::cs_busy = 4;
uint8_t ConnStatus::cs_validating = 5;
uint8_t ConnStatus::cs_idle_ready = 6;

// two reasons for this function:
// 1- "std::atomic_compare_exchange_strong" is too long to type/read.
// 2- contrary to POLS, "exchange" will store obj's value into 'expected' on inequality,
//    which is contrary to behavior I want.
template<typename _T>
inline bool atomicCAS(std::atomic<_T> *value, _T *expected, _T new_value)
{
	_T test = *expected;
	return std::atomic_compare_exchange_strong(value, &test, new_value);
}

// Query read result check.
void uvpg_read_result(uv_poll_t *poll, int status, int events)
{
	if(status == 0 && events == UV_READABLE)
	{
		// got some sort of readable event.  let's find out.
		int pgres;
		uvpg_result *result = (uvpg_result *)poll->data;
		assert(result != NULL);
		UVPGConnEntry *entry = result->entry;
		pgres = PQconsumeInput(entry->conn);
		if(pgres == 0)
		{
			// trouble consuming.
			uv_poll_stop(poll);
			//printf("DEBUG: PG error: %s\n", PQerrorMessage(entry->conn));
			if(result->failure_cb)
				result->failure_cb(entry->conn, result->data);
			else
				result->result_cb(entry->conn, result->data);
			poll->data = NULL;
			delete(result);
			return;
		}
		pgres = PQisBusy(entry->conn);
		if(pgres == 0)
		{
			// finished processing our request.  notify our caller.
			uv_poll_stop(poll);
			result->result_cb(entry->conn, result->data);
			poll->data = NULL;
			delete(result);
		}
		// ok, we checked the connection, still waiting for a result.
	}
}


// Connection poll methods.
static void uvpg_connection_poll(uv_poll_t *poll);

void uvpg_connection_poll_ready(uv_poll_t *pollhandle, int status, int events)
{
	uv_poll_stop(pollhandle);
	if(status == 0 && (events == UV_READABLE || events == UV_WRITABLE))
	{
		uvpg_connection_poll(pollhandle);
	}
}

static void uvpg_connection_poll(uv_poll_t *poll)
{
	uvpg_result *result = (uvpg_result *)(poll->data);
	UVPGConnEntry *entry = result->entry;
	UVPGPool *pool = (UVPGPool *)&(result->data);
	
	switch(PQconnectPoll(entry->conn))
	{
		case PGRES_POLLING_READING:
			// wait for socket to be readable
			uv_poll_start(&(entry->poller), UV_READABLE, uvpg_connection_poll_ready);
			break;
		case PGRES_POLLING_WRITING:
			uv_poll_start(&(entry->poller), UV_WRITABLE, uvpg_connection_poll_ready);
			break;
		case PGRES_POLLING_FAILED:
			pool->connectionFailed(entry);
			break;
		case PGRES_POLLING_OK:
			pool->connectionReady(entry);
			break;
		default:
			break;
	}
}

static void uvpg_connection_reset(uv_async_t *async, int status)
{
	UVPGPool *pool = (UVPGPool *)async->data;
	pool->checkIdleConnections();
}

//
// UVPGPool
//

UVPGPool::UVPGPool(uv_loop_t *in_loop, const char *in_connstring, unsigned in_min_connections, unsigned in_min_free_connections, unsigned in_max_free_connections)
: eventloop(in_loop), connstring(in_connstring), min_connections(in_min_connections),
min_free_connections(in_min_free_connections), max_free_connections(in_max_free_connections)
{
	createNewConnections(min_connections);
	uv_async_init(eventloop, &reset_msg, uvpg_connection_reset);
	reset_msg.data = this;
}
UVPGPool::~UVPGPool()
{
	size_t count = connections.size();
	for(int ix = 0; ix < count; ++ix)
	{
		disconnect(connections[ix]);
	}
}

void UVPGPool::watchConnectionState(UVPGConnEntry *entry)
{
	if(entry == NULL || entry->conn == NULL)
		return; // invalid connection to watch.
	
	uvpg_result *resstruct = newResultStruct();
	resstruct->entry = entry;
	resstruct->data	= this;
	entry->poller.data = resstruct;
	// now that we have our structure, poll the connection with PQconnectPoll.
	uvpg_connection_poll((uv_poll_t *)&entry->poller);
}

void UVPGPool::createNewConnections(unsigned newcount)
{
	// trying something a bit different, going to do exactly what Venom
	// suggested not to do: .push_back().
	if(newcount == 0)
		newcount = min_free_connections;
	
	size_t num_entries = connections.size();
	unsigned created_count = 0;
	for(size_t jx = 0; created_count < newcount && jx < num_entries; ++jx)
	{
		if(atomicCAS(&(connections[jx]->status), &(ConnStatus::cs_invalid), ConnStatus::cs_connecting))
		{
			created_count++;
			connections[jx]->conn = PQconnectStart(connstring);
			if(connections[jx]->conn)
			{
				if(PQstatus(connections[jx]->conn) == CONNECTION_BAD)
				{
					printf("Connection to database failed (%s): %s\n", connstring, PQerrorMessage(connections[jx]->conn));
				}
				else
				{
					watchConnectionState(connections[jx]);
				}
			}
		}
	}
	// now that we've grabbed all the inactive connections we can, create new ones until
	// we have enough free connections.
	for( ; created_count < newcount; ++created_count)
	{
		UVPGConnEntry *entry = new UVPGConnEntry;
		entry->conn = PQconnectStart(connstring);
		entry->status.store(ConnStatus::cs_connecting);
		connections.push_back(entry);
		if(entry->conn)
		{
			uv_poll_init_socket(eventloop, &(entry->poller), PQsocket(entry->conn));
			if(PQstatus(entry->conn) == CONNECTION_BAD)
			{
				printf("Connection to database failed (%s): %s\n", connstring, PQerrorMessage(entry->conn));
			}
			else
			{
				watchConnectionState(entry);
			}
		}
	}
}

void UVPGPool::connectionFailed(UVPGConnEntry *entry)
{
	// connection failed, so just remove it, and ... do we worry?
	if(atomicCAS(&(entry->status), &(ConnStatus::cs_connecting), ConnStatus::cs_disconnecting))
	{
		PQfinish(entry->conn);
		entry->conn = NULL;
		entry->status.store(ConnStatus::cs_invalid);
	}
}
void UVPGPool::connectionReady(UVPGConnEntry *entry)
{
	// connection has become ready, move it to our available connections queue.
	atomicCAS(&(entry->status), &(ConnStatus::cs_connecting), ConnStatus::cs_available);
	checkQueuedRequests();
}
void UVPGPool::checkIdleConnections()
{
	for(size_t ix = 0; ix < connections.size(); ++ix)
	{
		atomicCAS(&(connections[ix]->status), &(ConnStatus::cs_idle_ready), ConnStatus::cs_available);
	}
	checkQueuedRequests();
}

void UVPGPool::disconnect(PGconn *conn)
{
	UVPGConnEntry *entry = findConnEntry(conn);
	if(entry == NULL)
		return;
	disconnect(entry);
}
void UVPGPool::disconnect(UVPGConnEntry *entry)
{
	bool set_disconnect = false;
	if(atomicCAS(&(entry->status), &(ConnStatus::cs_connecting), ConnStatus::cs_disconnecting))
	{
		set_disconnect = true;
	}
	if(atomicCAS(&(entry->status), &(ConnStatus::cs_available), ConnStatus::cs_disconnecting))
	{
		set_disconnect = true;
	}
	if(atomicCAS(&(entry->status), &(ConnStatus::cs_busy), ConnStatus::cs_disconnecting))
	{
		set_disconnect = true;
	}
	if(set_disconnect && entry->status.load() == ConnStatus::cs_disconnecting)
	{
		// since once a status is set to disconnecting, that thread will be responsible for cleanup
		// and invalidation, this operation is fine here, as we set disconnect on this entry
		PQfinish(entry->conn);
		entry->conn	= NULL;
		entry->status.store(ConnStatus::cs_invalid);
		uv_poll_stop(&(entry->poller));
	}
}
UVPGConnEntry *UVPGPool::findConnEntry(PGconn *conn)
{
	if(!conn)
		return NULL;
	size_t count = connections.size();
	for(size_t ix = 0; ix < count; ++ix)
	{
		if(connections[ix]->conn == conn)
			return connections[ix];
	}
	return NULL;
}

// routines for getting a connection, and getting rid of it (because you're done).
PGconn *UVPGPool::getFreeConn(bool add_more)
{
	// grab the first available connection (least recently used)
	// shove it onto the busy list.
	PGconn *nextconn = NULL;
	size_t count = connections.size();
	for(unsigned ix = 0; ix < count; ++ix)
	{
		if(atomicCAS(&(connections[ix]->status), &(ConnStatus::cs_available), ConnStatus::cs_busy))
		{
			nextconn = connections[ix]->conn;
			break;
		}
	}
	if(nextconn == NULL)
	{
		return NULL;
	}
	
	// check connection status, make sure it hasn't gone bad.
	switch(PQstatus(nextconn))
	{
		case CONNECTION_BAD:
			// end this connection, and get a new one.
			PQfinish(nextconn);
			createNewConnections();
			nextconn = getFreeConn(add_more);
		default:
			break;
	}
	
	// see if we need to create a new connection, if we're full.
	if(add_more)
	{
		size_t ccount = connections.size();
		unsigned free_count = 0;
		for(size_t ix = 0; ix < ccount; ++ix)
		{
			int connstatus = connections[ix]->status.load();
			if(connstatus == ConnStatus::cs_available ||
			   connstatus == ConnStatus::cs_connecting)
				free_count++;
		}
		if(free_count < min_free_connections)
		{
			createNewConnections();
		}
	}

	return nextconn;
}
void UVPGPool::returnConnection(PGconn *in_conn)
{
	// remove connection from busy list, add to available.
	// alternative: call PQresetStart, and add to "connecting" list.
	UVPGConnEntry *entry = findConnEntry(in_conn);
	if(entry == NULL)
		return;
	if(atomicCAS(&(entry->status), &(ConnStatus::cs_busy), ConnStatus::cs_validating))
	{
		uv_poll_stop(&(entry->poller)); // just in case it's in the middle of anything.
		
		// clean up the connection
		PGresult *res = PQgetResult(entry->conn);
		while(res != NULL) {
			PQclear(res);
			res = PQgetResult(entry->conn);
		}
		entry->poller.data = NULL;
		
		// check the PQstatus to make sure it's not actually in the middle of anything
		// and that we haven't been lied to.  If we've been lied to, set to connecting & reset.
		switch(PQtransactionStatus(entry->conn))
		{
			case PQTRANS_IDLE:
				// this one is idle.
				entry->status.store(ConnStatus::cs_idle_ready);
				uv_async_send(&reset_msg);
				break;
			default:
				PQreset(entry->conn);
				entry->status.store(ConnStatus::cs_connecting);
				watchConnectionState(entry);
		}
	}
}

// various handling routines for how to execute a callback when a result comes in.
// they ultimately all call the first (using a uvpg_result_t *)
void UVPGPool::executeOnResult(uvpg_result *result, uvpg_result_cb callback, uvpg_result_cb failure_cb)
{
	result->result_cb = callback;
	result->failure_cb = failure_cb;
	// don't really like doing this circular set of pointers, but... sort of need it. (refactor, maybe?)
	result->entry->poller.data = result;
	
	uv_poll_start(&(result->entry->poller), UV_READABLE, uvpg_read_result);
}
void UVPGPool::executeOnResult(PGconn *in_conn, uvpg_result_cb callback, uvpg_result_cb failure_cb)
{
	uvpg_result *result = newResultStruct();
	result->entry = findConnEntry(in_conn);
	executeOnResult(result, callback, failure_cb);
}
void UVPGPool::executeOnResult(PGconn *in_conn, void *data, uvpg_result_cb callback, uvpg_result_cb failure_cb)
{
	uvpg_result *result = newResultStruct();
	result->entry = findConnEntry(in_conn);
	result->data	= data;
	
	executeOnResult(result, callback, failure_cb);
}

void UVPGPool::sendQueryAndDo(const char *query, UVPGParams *params, int resultFormat, void *data, uvpg_result_cb callback, uvpg_result_cb failure_cb)
{
	// try to get a free connection
	PGconn *conn = getFreeConn();
	// if success, do PQsendQueryParams,
	if(conn)
	{
		PQsendQueryParams(conn, query, (int)params->size(), params->oids(), params->values(), params->lengths(), params->formats(), resultFormat);
		executeOnResult(conn, data, callback, failure_cb);
	}
	// if failure, queue request up, and wait for free connection.
	else
	{
		UVPGQuery *pgquery = new UVPGQuery;
		pgquery->query = strdup(query);
		pgquery->params = new UVPGParams(*params);
		pgquery->resultFormat = resultFormat;
		pgquery->userdata = data;
		pgquery->callback = callback;
		pgquery->failure_cb = failure_cb;
		printf("Adding pending query\n");
		pendingQueries.push(pgquery);
	}
}

void UVPGPool::checkQueuedRequests()
{
	// check if we have any queued requests, and try to execute them.
	while(!pendingQueries.empty())
	{
		// try to get a free connection
		PGconn *conn = getFreeConn(false);
		if(conn)
		{
			// execute this pending query.
			UVPGQuery *pgquery = pendingQueries.front();
			pendingQueries.pop();
			UVPGParams *params = pgquery->params;
			PQsendQueryParams(conn, pgquery->query, (int)params->size(), params->oids(),
							  params->values(), params->lengths(), params->formats(), pgquery->resultFormat);
			executeOnResult(conn, pgquery->userdata, pgquery->callback, pgquery->failure_cb);
			delete params;
			delete pgquery;
		}
		else
			break;
	}
}
