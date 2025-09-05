# Poker Simulator (C)

### Overview
This project is a client-server poker application built in C using systems-level programming and socket communication. The goal is to create a multi-client poker game that operates in a local network environment. The server manages game state, handles player actions, and facilitates communication between all connected clients.

---

### Project Status
This project is a work in progress. The current implementation focuses on the core client-server architecture and game flow. Several key features are still under active development.

---

### Features (WIP)
-   **Client-Server Architecture**: The application is built with a clear separation between the server, which manages game logic, and up to 6 concurrent clients.
-   **State Machine**: A finite-state machine is used to manage the flow of the game, including player turns and betting rounds.
-   **Network Communication**: Leverages TCP sockets for reliable, bi-directional communication between the server and all clients.
-   **Fault Tolerance**: Designed to handle and recover from graceful client disconnections.

---

### How to Build and Run
This project requires a C compiler and the standard C library.

1.  Clone the repository:
    ```bash
    git clone [your-repo-link]
    ```
2. Compile the code:
    ```bash
    make server.poker_server
    ```
3.  Navigate to the project directory and build the server and client executables using `make`.
    ```bash
    make tui.client

    make client.automated
    ```
4.  Run the server: <br>
   '#' is a random number used to randomize the deck of cards.
    ```bash
    ./build/server.poker_server #
    ```
6.  Run a client: <br>
   '#' is the plyaer number between 0 and 5.
    ```bash
    ./build/client.poker_client #
    ```

---
