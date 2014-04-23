//
//  main.cpp
//  uv_postgresql_test
//
//  Created by Joseph Love on 3/14/14.
//  Copyright (c) 2014 Gameowls. All rights reserved.
//

#include <uv.h>
#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(_MSC_VER)
#include <windows.h>
#include <time.h>
#endif

#include "UVPGPool.h"
#include "UVPGParams.h"

// standard PostgreSQL connection string.
#define DB_CONNINFO "postgresql://testuser:testpass@localhost/mydatabase"

UVPGPool *connpool;

int counter = 0;

void pg_result_cb(PGconn *conn, void *data)
{
	PGresult *result = PQgetResult(conn);
	printf("Got result, %d records.  Error? %s\n", PQntuples(result), PQerrorMessage(conn));
	printf("Got result status: %d\n", PQresultStatus(result));
	
	PQclear(result);
	connpool->returnConnection(conn);
	counter--;
}

int periodic_counter = 0;

void on_periodic_timer(uv_timer_t *handle, int status)
{
	printf("\n********** Starting new run (%d) of periodic data\n", ++periodic_counter);
	printf("%d queries in progress\n", counter);
	if(counter >= 30) {
		printf("Too many queries in progress for my tastes\n");
		return;
	}
	counter++;
	UVPGPool *pool = (UVPGPool *)handle->data;
	for(int ix = 0; ix < 10; ++ix)
	{
		// character test
		UVPGParams params(1);
		params.add("Joe");
		
		pool->sendQueryAndDo("SELECT userid, password, suspended, extra_permissions "
							 "FROM users WHERE UPPER(username) = UPPER($1)",
							 &params, 0, NULL, pg_result_cb);
		//int res = PQsendQueryParams(conn,
		//							"SELECT userid, password, suspended, extra_permissions "
		//							"FROM users WHERE UPPER(username) = UPPER($1)",
		//							(int)params.size(), NULL, params.values(), params.lengths(), params.formats(), 1);
		//printf("result: %d\n", res);
		//pool->executeOnResult(conn, pg_result_cb);
	}
}


int main(int argc, const char * argv[])
{
	unsigned timer_first_ms = 1000;
	unsigned timer_repeat_ms = 10;
	uv_loop_t *loop = uv_default_loop();
	
	printf("Creating DB connection pool\n");
	connpool = new UVPGPool(loop, DB_CONNINFO);
	printf("\tInformation:\n");
	printf("\t%d PGRES_TUPLES_OK\n",		PGRES_TUPLES_OK);
	printf("\t%d PGRES_SINGLE_TUPLE\n",		PGRES_SINGLE_TUPLE);
	printf("\t%d PGRES_COMMAND_OK\n",		PGRES_COMMAND_OK);
	printf("\t%d PGRES_EMPTY_QUERY\n",		PGRES_EMPTY_QUERY);
	printf("\t%d PGRES_FATAL_ERROR\n",		PGRES_FATAL_ERROR);
	printf("\t%d PGRES_NONFATAL_ERROR\n",	PGRES_NONFATAL_ERROR);
	printf("\t%d PGRES_BAD_RESPONSE\n",		PGRES_BAD_RESPONSE);

	uv_timer_t periodic_routines;
	periodic_routines.data = connpool;
	
	uv_timer_init(loop, &periodic_routines);
	printf("Init timer for %0.4fs increments\n", (double)timer_repeat_ms/1000);
	uv_timer_start(&periodic_routines, on_periodic_timer, timer_first_ms, timer_repeat_ms);

	printf("Running\n");
	uv_run(loop, UV_RUN_DEFAULT);
	
    return 0;
}

