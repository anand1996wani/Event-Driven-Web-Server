# Event-Driven-Web-Server
Event Driven Web Server

## Objective  
Single process event driven model for web server. The web server accepts connections, processes http requests and responds with contents if the resource is found

## Design

`server.c`
*	Single process Web Server handling 1000 clients simultainously. In this web server HTTP request is processed in stages. Each request is divided into multiple stages such as accepting the connection, reading request, processing request, sending the result etc. These stages are not long running so that latency of responses experienced by clients will be minimum.

*	It uses the standard non-blocking read, write, and accept system calls on sockets, and the
select system call to test for I/O completion.

*	The web server also handles dynamic requests using FastCGI interface. Any request coming with an extension .cgi should go to FastCGI process and it will execute a file with that name. The output will is sent back to the client via the web server.

*	When the server has to read a file not present in memory, it uses helper processes for reading files. When loading the file is done, it indicates the server process. The number of helper processes is fixed.

* using select to check for I/O events on sockets
* using a single listening socket to accept connections
* each connection has a state maintained in the data store and a non-blocking socket in the select listener
* `READ_STATE` the process keeps reading data until it finds end of request
* file is mapped to memory using mmap, `404 Not Found` is sent back if file does not exist
* `WRITE_STATE` the process writes data from memory mapped file after which the socket is closed and the state data deleted

## Results
*	processing 1000 http req/s simultainously without failing.
*	HTTPERF results are shown
