# C++ IRC Server

![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![42 School](https://img.shields.io/badge/42%20Project-IRC-black?style=for-the-badge&logo=42)
![1337 School](https://img.shields.io/badge/1337%20Project-IRC-black?style=for-the-badge&logo=1337)
![RFC](https://img.shields.io/badge/RFC-0000FF?style=for-the-badge&logo=rfc&logoColor=white)
![epoll](https://img.shields.io/badge/epoll-00FF00?style=for-the-badge&logo=linux&logoColor=black)
![Socket](https://img.shields.io/badge/Sockets-FF0000?style=for-the-badge&logo=socket.io&logoColor=white)

## Overview

This project is a C++ implementation of an Internet Relay Chat (IRC) server, built from the ground up. An IRC server is a network daemon that facilitates real-time text-based communication between clients. Users connect to the server, join channels, and can then chat with other users in those channels or in private messages.

This server is designed to be non-blocking and scalable, capable of handling numerous simultaneous client connections efficiently. It adheres to the core principles of the IRC protocol as defined in the relevant RFCs.

***

## Platform Specificity: Linux Only

**This IRC server is designed to run exclusively on Linux.**

This is a deliberate design choice based on the use of the `epoll` API for handling network I/O. While other multiplexing mechanisms like `select` and `poll` are more portable, `epoll` offers significant performance advantages that are essential for a high-performance, real-time chat server that must handle many concurrent, long-lived connections.

## Table of Contents
- [Core Concept: `epoll` vs. `select`/`poll`](#core-concept-epoll-vs-selectpoll)
- [IRC Server Architecture](#irc-server-architecture)
- [Key Components Explained](#key-components-explained)
- [IRC Protocol and RFC Compliance](#irc-protocol-and-rfc-compliance)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
- [License](#license)
- [Author](#author)
- [Acknowledgments](#acknowledgments)

***

## Core Concept: `epoll` vs. `select`/`poll`

To handle many clients at once without creating a thread for each one, servers use I/O multiplexing. `select`, `poll`, and `epoll` are all APIs for this, but they work very differently.

### The Problem with `select` and `poll`

`select` and `poll` are level-triggered and operate in a way that is inefficient at scale. Every time you call them, you must pass the *entire list* of file descriptors (sockets) you are monitoring. The kernel then iterates through this entire list to check which ones are ready for I/O. This becomes a major bottleneck as the number of connections grows. The complexity is **O(n)**, where `n` is the number of connections.

```
// The Inefficient "Ask Everyone" Method (select/poll)

Your App: "Hey Kernel, out of these 10,000 sockets, is anyone ready?"
   |
   v
Kernel:   (Iterates through all 10,000 sockets)
          "Okay, after checking all of them, sockets 3, 42, and 1000 are ready."
   |
   v
Your App: (Processes the 3 ready sockets)
   |
   v
Your App: "Okay, how about now? Out of these 10,000 sockets..." (repeats)
```

### The `epoll` Advantage

`epoll` is an edge-triggered mechanism. You create a single `epoll` instance in the kernel and register the file descriptors you are interested in *once*. The kernel then watches these for you. When a socket becomes ready, the kernel adds it to a "ready list." When you call `epoll_wait`, the kernel simply gives you that list of already-known ready sockets. The complexity is **O(1)**, making it vastly more scalable and performant.

```
// The Efficient "Tell Me When Ready" Method (epoll)

Your App: "Hey Kernel, create an epoll instance for me."
   |
   v
Your App: "Watch these 10,000 sockets and just let me know if any become ready."
   |
   v
Kernel:   (Monitors sockets internally. Sockets 3, 42, and 1000 become ready.)
          (Kernel adds them to a private "ready list".)
   |
   v
Your App: "Hey Kernel, is anyone on the ready list?"
   |
   v
Kernel:   "Yep, here are sockets 3, 42, and 1000." (Returns immediately)
```
This efficiency is why `epoll` is the standard for high-performance networking on Linux.

***

## IRC Server Architecture

The server is built around a central event loop powered by `epoll`. It maintains the state of all connected clients and channels, processing commands as they arrive.

```
+---------------------+
|   Server Startup    |
| - Create epoll fd   |
| - Bind listening sock|
+---------------------+
          |
          v
+---------------------+
|   Main Event Loop   |
|   (epoll_wait)      |
+---------------------+
          |
+------------------------------------------+
|                                          |
v                                          v
[ New Connection Event ]                 [ Client Data Event ]
  - accept() new client                    - read() data from client socket
  - Create Client object                   - Append to client's buffer
  - Add client fd to epoll                 - Check for complete commands (\r\n)
                                           - If complete:
                                               |
                                               v
                                     +--------------------+
                                     | Command Processor  |
                                     | - Parse command    |
                                     | - Check permissions|
                                     | - Execute command  |
                                     +--------------------+
                                               |
                                               v
                                     +--------------------+
                                     |  State Management  |
                                     | - Update Client    |
                                     | - Update Channel   |
                                     | - Send replies     |
                                     +--------------------+
```

***

## Key Components Explained

The server's logic is broken down into several key classes that manage different aspects of the IRC protocol.

* **Server:** The core class that orchestrates everything. It initializes the listening socket, runs the main `epoll` event loop, and manages collections of clients and channels. It is responsible for accepting new connections and dispatching events.

* **Client:** Represents a single user connected to the server. Each `Client` object stores stateful information, such as:
    * The client's file descriptor (socket).
    * A buffer for incoming and outgoing messages.
    * Nickname, username, and real name.
    * Authentication status (whether they have provided the correct password).
    * A list of channels they have joined.

* **Channel:** Represents a chat room. Each `Channel` object maintains its own state, including:
    * The channel name (e.g., `#general`).
    * A list of clients (users) currently in the channel.
    * Channel modes (e.g., invite-only, topic protection).
    * The channel topic.
    * A list of operators (`@`).

* **Command Processor / Parser:** This is the logic that interprets raw messages from clients. When a complete command (e.g., `JOIN #channel`) is received, this component is responsible for:
    1.  Parsing the command and its arguments.
    2.  Verifying that the client has the necessary permissions (e.g., is authenticated, is a channel operator).
    3.  Executing the command's logic by interacting with the `Client` and `Channel` objects.
    4.  Generating and sending the appropriate numeric replies or messages to the relevant clients.

***

## IRC Protocol and RFC Compliance

This server aims to comply with the IRC protocol specifications, primarily **RFC 2812** ("Internet Relay Chat: Client Protocol").

Key commands and features implemented include:

* **Connection Registration:** `PASS`, `NICK`, `USER` for authenticating and identifying a new client.
* **Channel Operations:** `JOIN` to enter a channel, `PART` to leave, `TOPIC` to view or set the channel topic, `NAMES` to list users in a channel.
* **Messaging:** `PRIVMSG` for sending messages to channels or other users, `NOTICE` for sending non-reply notices.
* **User Modes:** `MODE` command to set user or channel modes.
* **Server Queries:** `PING`/`PONG` to maintain connection health, `QUIT` to disconnect.

***

## Project Structure

The project is organized by feature into separate directories, promoting a clean and modular codebase.

```
irc_server/
├── Makefile
├── includes/
└── srcs/
    ├── main.cpp
    ├── parsing/
    ├── server/
    ├── user/
    └── utils/
```

* `Makefile`: Compiles the project into an `ircserv` executable.
* `includes/`: Contains all the C++ header files (`.hpp`) for the project.
* `srcs/`: Contains all the C++ source files (`.cpp`).
    * `main.cpp`: The entry point. Parses command-line arguments and starts the server.
    * `parsing/`: Logic for parsing raw commands from clients.
    * `server/`: Core server logic, including the main loop, channel management, and network handling.
    * `user/`: Code related to client/user management.
    * `utils/`: Miscellaneous utility functions used across the project.

***

## Getting Started

### Prerequisites

* A C++ compiler (e.g., `g++` or `clang`)
* `make` utility
* A **Linux** operating system
* An IRC client (e.g., Irssi, HexChat) to connect to the server.

### Installation & Compilation

1.  **Clone the repository:**
    ```bash
    git clone [https://github.com/achahid19/irc_server.git](https://github.com/achahid19/irc_server.git)
    cd irc_server
    ```

2.  **Compile the project:**
    ```bash
    make
    ```

3.  **Run the server:**
    The server requires a port and a password as arguments.
    ```bash
    ./ircserv <port> <password>
    ```
    For example: `./ircserv 6667 mysecretpassword`

4.  **Connect with an IRC Client:**
    Using your client, connect to `127.0.0.1` (or your server's IP) on the specified port. You will need to provide the server password first with the `PASS` command:
    ```
    /connect 127.0.0.1 6667
    /pass mysecretpassword
    /nick your_nickname
    /user your_username 0 * :Your Real Name
    ```

***

## License

This project is academic work for the 42 Network and is not intended for commercial use or redistribution.

## Authors
**© Anas Chahid ksabi **@KsDev**** - [achahid19](https://github.com/achahid19)

**© Salah Eddine** - [Gama009](https://github.com/Gama099)

## Acknowledgements
This project is part of the curriculum at **1337 Coding School**, a member of the **42 Network**. It provides a deep, practical understanding of network programming, socket management, and stateful protocol implementation.

[![School](https://img.shields.io/badge/Notice-1337%20School-blue.svg)](https://1337.ma/en/)
[![School](https://img.shields.io/badge/Notice-42%20School-blue.svg)](https://42.fr/en/homepage/)
