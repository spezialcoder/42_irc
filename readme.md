# 42_IRC: Modern C++ IRC Chat Server 🚀

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg) ![IRC](https://img.shields.io/badge/Protocol-IRC-blueviolet) ![Student Project](https://img.shields.io/badge/Type-Student%20Project-brightgreen)

---

## 📑 Table of Contents
- [Project Overview](#project-overview)
- [Setup & Installation](#setup--installation)
- [Server Output](#server-output)
- [Connecting with WeeChat](#connecting-with-weechat)
- [Testing & Capabilities](#testing--capabilities)
- [Ideas for Further Exploration](#ideas-for-further-exploration)
- [About](#about)

---

## 📝 Project Overview

**42_IRC** is a student-built, modern C++ implementation of a classic IRC (Internet Relay Chat) server. It brings the legacy IRC protocol — one of the oldest and most robust chat standards — into a clean, maintainable, and extensible codebase. The project demonstrates network programming, protocol parsing, and multi-client management using C++17, with a focus on clarity and educational value.

> **What is IRC?**
> 
> IRC is a text-based chat protocol dating back to 1988, still used for group and private messaging. It is:
> - **Open and well-documented**
> - **Line-based**: Each command/message is a line ending with `\r\n`
> - **Stateless**: Clients can connect, join channels, and chat without persistent accounts
> - **Extensible**: Supports custom commands and server features

### ✨ Project Specialties
- **C++17, modular design**
- **Custom event-driven server core** (no external IRCd)
- **Handles multiple clients and channels**
- **Implements core IRC commands** (NICK, USER, JOIN, PART, PRIVMSG, etc.)
- **Password-protected server** (see below)
- **Verbose debug output for learning and troubleshooting**

---

## ⚡ Setup & Installation

### 1. Clone the Repository
```bash
git clone https://github.com/spezialcoder/42_irc.git
cd 42_irc
```

### 2. Build the Server
Requires: `g++`, `make`
```bash
make
```

### 3. Run the Server
```bash
./ircserv
```

- Default port: **6666** (see `main.cpp` to change)
- Default server password: **abc** (see `main.cpp`)
- Default server name: **irc.LeMaDa.hn** (see `main.cpp`)
- Leave terminal open while running the server
- Use Ctrl+C to stop the server gracefully

> 💡 **Customization:**
> - **Port**: Edit `constexpr int PORT = 6666;` in `main.cpp`
> - **Password**: Edit `constexpr auto SERVER_PASSWORD = "abc";` in `main.cpp`
> - **Server name**: Edit `constexpr auto SERVER_NAME = ...` in `main.cpp`

---

## 🖥️ Server Output

The server prints status and debug info to the console:
- `[MPlexServer][timestamp]` — Core server events
- `[CONNECT]` / `[DISCONNECT]` — Client connections
- `[MSG]` — Parsed IRC commands and arguments
- `[LOGIN]` — Registration and login status
- `[SERVER] Alive - Uptime: ...` — Heartbeat

![Server Console Output](pics/server_output.png)

---

## 💬 Connecting with WeeChat

1. **Install WeeChat**
   ```bash
   sudo apt install weechat
   # or use your OS package manager
   ```
2. **Start WeeChat**
   ```bash
   weechat
   ```
3. **Add the IRC server**
   In WeeChat, type:
   ```
   /server add 42irc *ask project team*/6666 -password=*easy as ...*
   /connect 42irc
   ```
   
   ![WeeChat Connection Screenshot](pics/connect_success.png)

4. **Join a channel**
   ```
   /join #channel
   ```
   With `/join #channel` you can join existing channels or create new ones if that name doesn't already exist.
   
   ![WeeChat Join Screenshot](pics/join_success.png)

---

## 🧪 Testing & Capabilities

- **Multiple clients**: Connect from several terminals or machines
- **Channel management**: `/join #channel`, `/part #channel`
- **Private messaging**: `/msg nick message`
- **Server password**: Try connecting with/without the correct password
- **Custom server name/port**: Change in `main.cpp` and rebuild
- **Observe debug output**: Watch how the server parses and responds to commands

---

## 🚀 Ideas for Further Exploration
- Implement more IRC commands (MODE, TOPIC, KICK, etc.)
- Add logging to file
- Add SSL/TLS support
- Build a web-based IRC client
- Experiment with bots or automation

## 📚 Resources
- https://modern.ircdocs.horse/ Modern IRC Documentation
- https://tools.ietf.org/html/rfc1459 RFC 1459 - Internet Relay Chat Protocol
- https://en.wikipedia.org/wiki/List_of_IRC_commands - List of IRC Commands
- various AI assistants were used to help us understand the protocol, test for compliance, and brainstorm features. We also used them to generate some of the documentation design and comments in the code.

---

## 👩‍💻 About
This project was created as a student exercise in network programming and protocol design. It is intended for learning, experimentation, and as a foundation for more advanced chat systems.

Lewin Sorg, Matthias Naumann, Daniel Springer
---

**Happy chatting! 🎉**
