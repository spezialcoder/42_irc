# MPlexServer — Minimal epoll-based server library

A small C++17 library providing a multiplexed TCP server built on Linux `epoll`. It accepts clients, reads messages, and notifies you via simple callback hooks.

This document is a quick, lean guide for colleagues integrating or hacking on the server.

## Key concepts
- Namespace: `MPlexServer`
- Core class: `Server` — owns the listening socket, `epoll` instance, and client registry.
- Client wrapper: `Client` — lightweight info about a connection (fd, remote IP, port).
- Message wrapper: `Message` — a chunk of bytes received from a `Client` (currently one `recv` per poll).
- Callback hooks: function pointers for connect/disconnect/message events.

## Public API (essentials)
Header: `server/include/mplexserver.h`

- Construction
  ```cpp
  Server::Server(uint16_t port, std::string ipv4 = "");
  ```
  - `port`: TCP port to listen on
  - `ipv4`: optional bind address; empty string binds to all interfaces

- Lifecycle
  ```cpp
  void Server::activate();   // bind, listen, epoll, set non-blocking
  void Server::deactivate(); // drops all clients, closes epoll and listen fd
  ```

- Polling (non-blocking tick)
  ```cpp
  std::vector<Message> Server::poll();
  ```
  - Returns messages received during this tick.
  - Internally uses `epoll_wait(..., 0)`, so it never blocks. Pace your loop externally (e.g., a small sleep) if desired.

- Verbosity and stats
  ```cpp
  void Server::setVerbose(int level); // 0..2
  int  Server::getVerbose() const;
  int  Server::getConnectedClientsCount() const;
  ```
  - Level 0: critical, 1: connects/disconnects, 2: data/debug.

- Event hooks
  ```cpp
  using EventHandler   = void(*)(Client client);
  using MessageHandler = void(*)(Message msg);

  void Server::setOnConnect(EventHandler handler);
  void Server::setOnDisconnect(EventHandler handler);
  void Server::setOnMessage(MessageHandler handler);
  ```
  - Handlers are optional. If not set, the server does nothing special besides returning messages from `poll()`.

- Client helpers
  ```cpp
  int         Client::getFd()   const;
  std::string Client::getIpv4() const; // dotted quad
  int         Client::getPort() const; // host order
  ```

- Message helpers
  ```cpp
  const Client&  Message::getClient()  const;
  std::string    Message::getMessage() const; // raw payload up to MAX_MSG_LEN
  ```

## Minimal usage example
```cpp
#include <iostream>
#include "server/include/mplexserver.h"

using namespace MPlexServer;

static void onConnect(Client c) {
    std::cout << "[onConnect] " << c.getIpv4() << ":" << c.getPort() << "\n";
}

static void onDisconnect(Client c) {
    std::cout << "[onDisconnect] fd=" << c.getFd() << "\n";
}

static void onMessage(Message m) {
    std::cout << "[onMessage] from fd=" << m.getClient().getFd()
              << ": " << m.getMessage() << "\n";
}

int main() {
    Server srv(6667);      // bind 0.0.0.0:6667
    srv.setVerbose(1);

    srv.setOnConnect(&onConnect);
    srv.setOnDisconnect(&onDisconnect);
    srv.setOnMessage(&onMessage);

    srv.activate();

    while (true) {
        // poll is non-blocking; returns newly received messages this tick
        auto msgs = srv.poll();
        // You can also handle messages directly here instead of using onMessage
        for (const auto& msg : msgs) {
            // app-level handling
        }
        // Optional pacing to reduce CPU in idle state
        usleep(10 * 1000); // 10ms
    }
}
```

## Behavior notes
- Non-blocking everywhere:
  - Listening socket is non-blocking.
  - Accepted client sockets are set to non-blocking.
  - `poll()` uses `epoll_wait(..., 0)` and will not block; design your main loop accordingly.
- Disconnects:
  - Clean EOF (`recv` == 0) or certain errors will remove the client and invoke disconnect handler.
- Limits/defines (see header):
  - `MAX_MSG_LEN` (default 512) — maximum bytes read per `recv` into a single `Message`.
  - `MAX_EPOLL_EVENTS` (default 10) — max events processed per `epoll_wait` call.
- Message framing:
  - Current implementation treats each `recv` buffer as a full message; for IRC-style `\r\n` framing you can parse the returned payloads or attach a higher-level parser.
