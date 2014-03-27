# UVPGPool

UVPGPool is a C++ class which presents a very basic [PostgreSQL](http://postgresql.org) Connection Pool.  It uses libpq's asynchronous functions for connections, and expects the user to use the associated asynchronous query functions.  It utilizes [libuv](https://github.com/joyent/libuv) to watch for the connection and query responses, so that those can be handled asynchronously.

It has an additional dependency on C++11 for std::atomic.

UVPGParams is an optional class which I use to simplify my life when using PQsendQueryParams.

## Using UVPGPool

Please note that main.cpp is just a test file, and sort of an example of usage.  It's basically what I used to test that this was working.

This really isn't a library, just a couple of classes and header files.  Expectations are that they would be copied into your own project.  You will need to compile with both libuv & libpq headers & libraries (which you would presumeably need anyway, prior to this).  After all, this class sort of only makes sense if you're using both of those already.

Basic jist:

1. Create UVPGPool (requires uv loop)
2. getFreeConn();
3. Send asynchronous PQ-Query.
4. executeOnResult(connection, callback)
5. PQgetResult()
6. Repeat 3-5 as needed.
7. returnConnection().

Connections are sort of held hostage by your code.  It is sort of possible to stave the pool by never returning connections in a timely fashion.

The UVPGParams class is optional (to use).  I used it to simplify my life when using `PQsendQueryParams` as it meant I could just create an object which would put the associated lengths and formats (and tries to do Oids) into a single space, and then get the data back out with just a couple function calls.


## Notes

I got this question from a friend of mine:  Why do you need std::atomic if you're not currently using threads?

Well, I wanted to allow this system to be (sortof) thread-safe.  I intended to use this in a peice of code which is not currently using any asynchronous calls, and so the easiest option for lengthy operations (which are basically IO intensive) would be to push those off to a thread.  So the next thing I'd know is that I would have threads which are trying to get connections and return connections to the pool.  So, my two options were either a mutex (which would wrap way too many operations) or an atomic variable representing a connection's known state.  I chose the atomic variable, because it was something new for me to learn.

## Todo:

- Need to watch for failed connections, especially when we try to create too many connections.
- Add a callback mechanism for initial connection pool being "ready" (eg, connected)
- More reponses for failures (eg, no connection available, connections failed).
- Verify that float/double parameters in UVPGParams actually work (completely untested).
- Make it possible to have the Query responses read by a different loop.
- Fix validation of connections which are returned to the pool.  Right now, PQreset() ends up getting called on all connections.