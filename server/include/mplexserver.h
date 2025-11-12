/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsorg <lsorg@student.42heilbronn.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 18:57:28 by lsorg             #+#    #+#             */
/*   Updated: 2025/11/12 02:14:20 by lsorg            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

#define VERBOSITY_MAX 2
#define MAX_EPOLL_EVENTS 10
#define MAX_MSG_LEN 512

namespace MPlexServer {
    /**
     * @brief General server errors.
     */
    class ServerError final : public std::runtime_error {
    public:
        explicit ServerError(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    /**
     * @brief Server errors related to it's settings.
     */
    class ServerSettingsError final : public std::runtime_error {
    public:
        explicit ServerSettingsError(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    /**
     *
     * @param fd File descriptor to be set non blocking.
     */
    void setNonBlocking(int fd);

    /**
     * @brief Client class containing information about a client.
     */
    class Client {
    public:
        Client();
        Client(const Client& other);
        Client& operator=(const Client& other);

        explicit Client(int fd, sockaddr_in addr);

        [[nodiscard]] int getFd() const;
        [[nodiscard]] std::string getIpv4() const;
        [[nodiscard]] int getPort() const;

        ~Client();
    private:
        int fd;
        sockaddr_in client_addr{};
    };

    /**
     * @brief Message class.
     */
    class Message final {
    public:
        Message();
        Message(std::string msg, Client client);
        Message(const Message& other);
        Message& operator=(const Message& other);

        [[nodiscard]] const Client& getClient() const;
        [[nodiscard]] std::string getMessage() const;
    private:
        std::string message;
        Client client;
    };

    class EventHandler {
    public:
        virtual void onConnect(Client client) = 0;
        virtual void onDisconnect(Client client) = 0;
        virtual void onMessage(Message msg) = 0;
    };
    enum class EventType {CONNECTED, DISCONNECTED, MESSAGE};
    /**
     * @bief Multiplexer Server class
     */
    class Server final {
    public:
        Server() = delete;
        Server(const Server& other) = delete;
        Server& operator=(const Server& other) = delete;

        /**
         * @brief Creates an MPlexServer class.
         * @param port Specifies the port for the server (port cannot be changed once set).
         * @param ipv4 Specifies the ipv4 address the server should bind to. (Default value binds to all available network interfaces)
         */
        explicit Server(uint16_t port, std::string ipv4="");

        ~Server();

        /**
         * @brief Activates the server.
         */
        void activate();

        /**
         * @brief Deactivates the server.
         *
         * Closes the socket and cuts all active connections.
         */
        void deactivate();

        /**
         * @return Returns the amount of clients currently connected to the server.
         */
        int getConnectedClientsCount() const;

        /**
         * @brief Sets the level of verbosity.
         * @param level Level of verbosity.
         *
         * Level 0: Critical messages.
         * Level 1: Client connections/disconnections
         * Level 2: Transmitted data, additionally debug information and everything from level 1.
         */
        void setVerbose(int level);

        /**
         * @return Returns the current verbosity level.
         */
        int getVerbose() const;

        /**
         * @brief Poll all clients and accept new clients.
         */
        void poll();

        /**
         * @brief Set the active eventhandler instance for the server.
         */
        void setEventHandler(EventHandler* handler);

        /**
         * @brief Transmits a text message to client c.
         * @param c Client to send to.
         * @param msg Message to send.
         */
        void sendTo(const Client& c, std::string msg);

        /**
         * @brief Write message to all connected clients.
         * @param message Message to send.
         */
        void broadcast(std::string message);

    private:
        int server_fd;
        const int port;
        const std::string ipv4;
        int verbose;
        int epollfd;
        int clientCount;
        std::unordered_map<int, Client> client_map;
        std::unordered_map<int, std::string> send_buffer;
        EventHandler* handler;

        void log(const std::string message, int required_level) const;
        void deleteClient(const int fd);
        void callHandler(EventType event, Client client, Message msg=Message()) const;
        void modifyEpollFlags(const int fd, const int flags);
    };
}

