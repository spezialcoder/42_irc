/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsorg <lsorg@student.42heilbronn.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 18:57:28 by lsorg             #+#    #+#             */
/*   Updated: 2025/11/10 22:57:25 by lsorg            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <chrono>
#include <iomanip>
#include <iostream>

#define VERBOSITY_MAX 2

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
    void setNonBlocking(const int fd);

    /**
     * @brief Client class containing information about a client.
     */
    class Client {
    public:
        Client() = delete;
        Client(const Client& other) = delete;
        Client& operator=(const Client& other) = delete;

        explicit Client(int fd, sockaddr_in addr);
        ~Client();

    private:
        const int fd;
        const sockaddr_in client_addr;
    };


    /**
     * @bief Multiplexer Server class
     */
    class MPlexServer final {
    public:
        MPlexServer() = delete;
        MPlexServer(const MPlexServer& other) = delete;
        MPlexServer& operator=(const MPlexServer& other) = delete;

        /**
         * @brief Creates an MPlexServer class.
         * @param port Specifies the port for the server (port cannot be changed once set).
         * @param ipv4 Specifies the ipv4 address the server should bind to. (Default value binds to all available network interfaces)
         */
        explicit MPlexServer(uint16_t port, std::string ipv4="");

        ~MPlexServer();

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
         * Level 0: No output.
         * Level 1: Client connections/disconnections
         * Level 2: Transmitted data, additionally debug information and everything from level 1.
         */
        void setVerbose(int level);

        /**
         * @return Returns the current verbosity level.
         */
        int getVerbose() const;


    private:
        int fd;
        const int port;
        const std::string ipv4;
        int verbose;
        int epollfd;

        void log(const std::string message, int required_level) const;

    };
}

