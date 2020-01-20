# simple-echo-server

A simple echo server write in C by using difference Linux system call for listening file descriptors.

Basically, this is a server that echoes everthing the client transmitted, while printing it out in the server's stdout. It's a very simple demonstration of how to implement a HIGH PERFORMANCE web server from the scratch.

## basic.c

The very basic socket server that only accepts and processes one connection at a single time.

1) `gcc basic.c -o basic.out` to compile.

2) `./basic.out` to run

3) `telnet 127.0.0.1 9999` in another terminal to connect and test.

## ppc.c

ppc -> process per connection

The socket server that handles multiple connections concurrently by forking sub processes for each client that comes in.

1) `gcc ppc.c csapp.c -o ppc.out` to compile.

2) `./ppc.out` to run

3) Try `telnet 127.0.0.1 9999` in multiple terminals to connect and test.

## prefork.c

The socket server that handles multiple connections concurrently by pre-forking a bunch of processes which all accept connection from the same socket. The pre-fork number is defined by the macro `PREFORK_PROC_N`. When one process finals the handling, it will exit out. The main process will be signaled for such event and spawns a new process to balance the number of processes.

1) `gcc prefork.c csapp.c -o prefork.out` to compile.

2) `./prefork.out` to run

3) Try `telnet 127.0.0.1 9999` in multiple terminals to connect and test.

## tpc.c

tpc -> thread per connection

The socket server that handles multiple connections concurrently by creating a thread for each coming connection. The accept happens in the main thread, then hands to the created thread to handle it. Meanwhile, as threads share the same memory space, this code exploit this advantage by having a global counter that counts how many characters have been received in total. And print this number to the stdout for each time of receiving.

1) `gcc -pthread tpc.c csapp.c -o tpc.out` to compile.

2) `./tpc.out` to run

3) Try `telnet 127.0.0.1 9999` in multiple terminals to connect and test.

## select.c

This server utilizes the `select` I/O multiplexing to concurrently handle multiple connections by a single thread within a single process. Also, this server accepts string from stdin to broadcast to all clients.

1) `gcc -pthread select.c csapp.c -o select.out` to compile.

2) `./select.out` to run. Type in stdin to send string. Send `q` to quit.

3) Try `telnet 127.0.0.1 9999` in multiple terminals to connect and test.

## kqueue.c

This server utilizes the `kqueue` and `kevent` I/O multiplexing to concurrently handle multiple connections I/O by a single thread within a single process. Also, this server accepts string from stdin.

Notice: `kqueue` and `kevent` is only available in BSD-like system.

1) `gcc -pthread kqueue.c csapp.c -o kqueue.out` to compile.

2) `./kqueue.out` to run. Type in stdin to send string. Send `q` to quit.

3) Try `telnet 127.0.0.1 9999` in multiple terminals to connect and test.

## poll.c

This server utilizes the `poll` I/O multiplexing to concurrently handle multiple connections by a single thread within a single process. Also, this server accepts string from stdin to broadcast to all clients.

The difference between `poll.c` and `select.c` is that `poll.c` supports a huge number of clients, more than `select.c`, however, at a cost of more time to re-arrange the clients array when disconnecting one. 

1) `gcc -pthread select.c csapp.c -o select.out` to compile.

2) `./select.out` to run. Type in stdin to send string. Send `q` to quit.

3) Try `telnet 127.0.0.1 9999` in multiple terminals to connect and test.