# CS F372 Operating Systems - Assignment 2

This is a client-server application for message passing and logging, which comprises of 1 server and "n" clients. The applications are meant to be running in a single system and exchange data using stateless communication (i.e., single message block should be passed and completed in a single communication loop). Every request from the clients to the server is replied to the client by the server. Every action is initiated by the client and acted upon by the server.

The system follows a request-response mechanism and provides for the following set of actions for the client and server:

- REGISTER: This is a client-initiated action where the client connects with the server in the server's connect channel, for the very first time with a unique name. The server will verify that the client's name is unique and unused, and return with a key (string). The server will allocate the comm channel, shared memory, and create the worker thread for the client. The key will be used as a common shared link for all further communication.
- UNREGISTER: This is also a client-initiated action where it disconnects from its allocated comm channel.

## Requirements

- POSIX compliant C compiler
- pthread library

## Usage

### Server

To run the server, compile `server.c` using any POSIX compliant C compiler:

```
gcc -o server server.c -lpthread
```

Then run `server`:

```
./server
```

The server will start listening for incoming connections from clients.

### Client

To run a client, compile `client.c` using any POSIX compliant C compiler:

```
gcc -o client client.c
```

Then run `client`:

```
./client
```

The client will prompt you to enter your name. After entering your name, you can send messages to other clients by entering their registration key and the message.

## Implementation Details

The server uses a shared memory segment to store information about the connected clients. The shared memory segment contains a mutex lock to synchronize access to the shared data, as well as an array of client structures that store information about each connected client.

When a client connects to the server, it sends a registration request containing its name. The server verifies that the name is unique and unused, and returns a registration key to the client. The registration key is used as a common shared link for all further communication between the client and server.

The server spawns a new thread for each connected client, which handles requests from that client. The thread waits for the mutex lock to be available, then checks if the client has disconnected. If the client is still connected, it handles the request from the client and releases the mutex lock.

The client sends messages to other clients by specifying their registration key and the message. The server forwards the message to the specified client by looking up its registration key in the shared data.

The server also supports unregistering clients, which removes them from the shared data and disconnects them from their allocated comm channel.

## Synchronization

The server uses a mutex lock to synchronize access to the shared data. Each thread that handles requests from a connected client acquires this lock before accessing or modifying any shared data.

## References

This project was developed as part of CS F372 Operating Systems course at BITS Pilani, Hyderabad Campus.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
