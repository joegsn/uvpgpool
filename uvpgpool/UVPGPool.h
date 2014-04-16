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
//  uv_postgresql.h
//  UVPGPool
//
//  Created by Joseph Love on 3/14/14.
//  Copyright (c) 2014 Gameowls. All rights reserved.
//

#ifndef __UVPGPool__
#define __UVPGPool__

//
// Purpose, is to enable an event-driven (libuv) database connection pool
// Everything in here is designed to be async, and will add/remove appropriate
// events from the provided loop.
//

#include <uv.h>
#include <libpq-fe.h>
#include <vector>
#include <atomic>
#include <cstdint>

typedef void (*uvpg_result_cb)(PGconn *conn, void *data);

class ConnStatus
{
public:
	static uint8_t cs_invalid, cs_disconnecting, cs_connecting, cs_available,
	cs_busy, cs_validating, cs_idle_ready;
};


// note: convert from disconnecting->invalid should only ever happen in a single thread.
//       Whoever gets the atomic operation to "disconnecting" should invalidate the entry.
class UVPGConnEntry
{
public:
	UVPGConnEntry() : conn(NULL) { status.store(ConnStatus::cs_invalid); };
	UVPGConnEntry(const UVPGConnEntry &rhs) : status(rhs.status.load()), conn(rhs.conn) { };
	std::atomic<uint8_t> status;
	PGconn *conn;
	uv_poll_t poller; // only one uv_poll_s per connection.
};

class uvpg_result
{
public:
	uvpg_result() : entry(NULL), data(NULL) { };
	UVPGConnEntry *entry;
	void *data;
	uvpg_result_cb result_cb;
	uvpg_result_cb failure_cb;
};

class UVPGPool
{
private:
	uv_loop_t *eventloop;
	const char *connstring;
	uv_async_t reset_msg;
	
	unsigned min_connections;
	unsigned min_free_connections;
	unsigned max_free_connections;
	
	std::vector<UVPGConnEntry *> connections;
	
	void watchConnectionState(UVPGConnEntry *newconn);
	void createNewConnections(unsigned count=0);
	void disconnect(PGconn *entry);
	void disconnect(UVPGConnEntry *entry);
	UVPGConnEntry *findConnEntry(PGconn *conn);
	
public:
	UVPGPool(uv_loop_t *in_loop, const char *in_connstring, unsigned in_min_connections=5, unsigned in_min_free_connections=2, unsigned in_max_free_connections=7);
	~UVPGPool();
	
	// routines used internally for a connection.
	void connectionFailed(UVPGConnEntry *entry);
	void connectionReady(UVPGConnEntry *entry);
	void checkIdleConnections();
	
	// routines for getting a connection, and getting rid of it (because you're done).
	PGconn *getFreeConn();
	void returnConnection(PGconn *in_conn);
	
	// various handling routines for how to execute a callback when a result comes in.
	// they ultimately all call the first (using a uvpg_result *)
	uvpg_result *newResultStruct() { return new uvpg_result; }
	void executeOnResult(uvpg_result *in_conn, uvpg_result_cb callback, uvpg_result_cb failure_cb=NULL);
	void executeOnResult(PGconn *in_conn, uvpg_result_cb callback, uvpg_result_cb failure_cb=NULL);
	void executeOnResult(PGconn *in_conn, void *data, uvpg_result_cb callback, uvpg_result_cb failure_cb=NULL);
};

#endif /* defined(__UVPGPool__) */
