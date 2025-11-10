/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lsorg <lsorg@student.42heilbronn.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 18:57:28 by lsorg             #+#    #+#             */
/*   Updated: 2025/11/10 20:52:25 by lsorg            ###   ########.fr       */
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
#include <iostream>

namespace MPlexServer {
    class ServerError final : public std::runtime_error {
    public:
        explicit ServerError(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };


    class Client {
    public:
        Client();
        ~Client();

    private:

    };

    class MPlexServer final {
    public:
        MPlexServer() = delete;
        MPlexServer(const MPlexServer& other) = delete;
        MPlexServer& operator=(const MPlexServer& other) = delete;

        explicit MPlexServer(uint16_t port);
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

    private:

    };
}

