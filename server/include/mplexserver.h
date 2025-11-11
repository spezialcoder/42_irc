/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsorg <lsorg@student.42heilbronn.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 18:57:28 by lsorg             #+#    #+#             */
/*   Updated: 2025/11/11 01:34:22 by lsorg            ###   ########.fr       */
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

        int getFd() const;
        std::string getIpv4() const;
        int getPort() const;

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

        void setMessage(std::string msg);
        void setClient(Client& client);
    private:
        std::string message;
        Client client;
    };

    enum class EventType {CONNECTED, DISCONNECTED, MESSAGE};
    using EventHandler = std::function<void(Client)>;
    using MessageHandler = std::function<void(Message)>;
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
         * @return Returns a list of newly received messages
         */
        std::vector<Message> poll();

        /**
         * @brief Sets a custom handler function which will be called when a new client connects.
         * @param handler function of type EventHandler (void(*)(Client client))
         */
        template<typename Callable, typename ... Args>
        void setOnConnect(Callable &&c, Args &&...args) {
            auto capturedArgs = std::make_tuple(std::forward<Args>(args)...);
            auto fn = std::forward<Callable>(c);

            handlers.onConnect = [fn = std::move(fn), capturedArgs=std::move(capturedArgs)](Client client) mutable {
                std::apply([&](auto&&... unpackedArgs) {
                    fn(client,std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
                },capturedArgs);
            };

        }


        /**
         * @brief Sets a custom handler function which will be called when a new client disconnects.
         * @param handler function of type EventHandler (void(*)(Client client))
         */
        void setOnDisconnect(EventHandler handler);

        /**
         * @brief  Sets a custom handler function which will be called whenever a message is send.
         * @param handler function of type EventHandler (void(*)(Message msg))
         */
        template<typename Callable, typename ... Args>
        void setOnMessage(Callable&& c, Args&&... args) {
            auto capturedArgs = std::make_tuple(std::forward<Args>(args)...);
            auto fn = std::forward<Callable>(c);

            handlers.onMessage = [fn = std::move(fn), capturedArgs=std::move(capturedArgs)](Message msg) mutable {
                std::apply([&](auto&&... unpackedArgs) {
                    fn(msg,std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
                },capturedArgs);
            };
        }

    private:
        using EventHandlers = struct {
            EventHandler onConnect;
            EventHandler onDisconnect;
            MessageHandler onMessage;
        };

        int server_fd;
        const int port;
        const std::string ipv4;
        int verbose;
        int epollfd;
        int clientCount;
        std::unordered_map<int, Client> client_map;
        EventHandlers handlers{};

        void log(const std::string message, int required_level) const;
        void deleteClient(int fd);
        void callHandler(EventType event, Client client, Message msg=Message()) const;
    };
}

