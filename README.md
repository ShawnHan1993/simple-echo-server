# simple-echo-server

A simple echo server write in C by using difference Linux system call for listening file descriptors

## basic.c

The very basic socket server that only accepts and processes one connection at a single time.

1) `gcc basic.c -o basic.out` to compile.

2) `./basic.out` to run

3) `telnet 127.0.0.1 9999` in another terminal to connect and test.

## ppc.c

The socket server that handles multiple connections concurrently by forking sub processes for each client that comes in.

1) `gcc ppc.c csapp.c -o ppc.out` to compile.

2) `./ppc.out` to run

3) Try `telnet 127.0.0.1 9999` in multiple terminals to connect and test.

## prefork.c

The socket server that handles multiple connections concurrently by pre-forking a bunch of processes which all accept connection from the same socket. The pre-fork number is defined by the macro `PREFORK_PROC_N`.

1) `gcc prefork.c csapp.c -o prefork.out` to compile.

2) `./prefork.out` to run

3) Try `telnet 127.0.0.1 9999` in multiple terminals to connect and test.