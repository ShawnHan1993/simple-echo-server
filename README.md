# simple-echo-server
A simple echo server write in C by using difference Linux system call for listening file descriptors


## basic.c

The very basic socket server that only accepts and processes one connection at a single time.

1) `gcc basic.c -o basic.out` to compile.

2) `./basic.out` to run

3) `telnet 127.0.0.1 9999` in another terminal to connect and test.

## ppc.c

The socket server that handles multiple connections concurrently at the same time by forking sub processes for each client that comes in.

1) `gcc ppc.c csapp.c -o ppc.out` to compile.

2) `./ppc.out` to run

3) Try `telnet 127.0.0.1 9999` in multiple terminals to connect and test.