#include "../include/mplexserver.h"

MPlexServer::Server::Server(uint16_t port, const std::string ipv4) : port(port), ipv4(ipv4) {
    this->verbose = 0;
    this->server_fd = -1;
    this->epollfd = -1;
    this->clientCount = 0;
}

MPlexServer::Server::~Server() {
    deactivate();
}

void MPlexServer::Server::activate() {
    int listen_fd = socket(AF_INET,SOCK_STREAM,0);
    if (listen_fd < 0) {
        throw ServerError("Failed to open socket");
    }
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(listen_fd);
        throw ServerError("Failed to set SO_REUSEADDR");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (this->ipv4 != "") {
        if (inet_pton(AF_INET, this->ipv4.c_str(), &addr.sin_addr.s_addr) <= 0) {
            close(listen_fd);
            throw ServerSettingsError("Invalid IPv4 address");
        }
    } else {
        addr.sin_addr.s_addr = INADDR_ANY;
    }

    if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        close(listen_fd);
        throw ServerError("Failed to bind socket");
    }

    if (listen(listen_fd, SOMAXCONN) == -1) {
        close(listen_fd);
        throw ServerError("Failed to listen socket");
    }
    setNonBlocking(listen_fd);
    const int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        close(listen_fd);
        throw ServerError("Failed to create epoll instance");
    }
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        close(listen_fd);
        close(epoll_fd);
        throw ServerError("Failed to add server fd to epoll instance");
    }

    this->epollfd = epoll_fd;
    this->server_fd = listen_fd;

    log("Server successfully activated",1);
}

void MPlexServer::Server::log(const std::string message, const int required_level) const {
    if (this->verbose >= required_level) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm = *std::localtime(&now_time);
        std::cout << "[MPlexServer][" << std::put_time(&local_tm, "%Y-%m-%d@%H:%M:%S") << "] " << message << std::endl;
    }
}

void MPlexServer::Server::deactivate() {
    for (const auto& [fd,c] : client_map) {
        if (epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,nullptr) == -1) {
            log("Critical error could not delete fd from epoll.",0);
        }
        close(fd);
    }
    this->clientCount = 0;
    this->client_map.clear();
    if (server_fd != -1) close(server_fd);
    if (epollfd != -1) close(epollfd);
    log("Server has been deactivated.",1);
}

void MPlexServer::Server::setVerbose(const int level) {
    if (level <= VERBOSITY_MAX && level >= 0) {
        this->verbose = level;
    } else {
        throw ServerSettingsError("Verbosity level does not exist");
    }
}

int MPlexServer::Server::getVerbose() const {
    return this->verbose;
}

int MPlexServer::Server::getConnectedClientsCount() const {
    return this->clientCount;
}

std::vector<MPlexServer::Message> MPlexServer::Server::poll() {
    struct epoll_event events[MAX_EPOLL_EVENTS];
    std::vector<Message> report{};
    int numEvents = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, 0);
    if (numEvents == -1) {
        throw ServerError("Failed to poll epoll events");
    }

    for (int i = 0; i < numEvents; ++i) {
        if (events[i].data.fd == this->server_fd) {
            struct sockaddr_in client_addr;
            socklen_t len = sizeof(client_addr);
            int clientFd;
            while ((clientFd = accept(server_fd, (struct sockaddr*)&client_addr,&len)) != -1) {
                len = sizeof(client_addr);
                setNonBlocking(clientFd);
                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLRDHUP;
                ev.data.fd = clientFd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientFd, &ev) == -1) {
                    close(clientFd);
                    log("Failed to add client to epoll.",0);
                    continue;
                }
                client_map[clientFd] = Client(clientFd, client_addr);
                clientCount++;
                log("New client accepted.",1);
                callHandler(EventType::CONNECTED,client_map[clientFd]);
            }

        } else {
            if (events[i].events & EPOLLIN) {
                char buffer[MAX_MSG_LEN];
                ssize_t n = recv(events[i].data.fd, buffer, MAX_MSG_LEN,0);
                if (n==0) {
                    log("Client disconnected.",1);
                    callHandler(EventType::DISCONNECTED,client_map[events[i].data.fd]);
                    deleteClient(events[i].data.fd);
                    continue;
                }
                if (n < 0) {
                    switch (errno) {
                        case EAGAIN:
                            break;
                        case ECONNRESET:
                            log("Connection of client has been reset",1);
                            callHandler(EventType::DISCONNECTED,client_map[events[i].data.fd]);
                            deleteClient(events[i].data.fd);
                            break;
                        case ETIMEDOUT:
                            log("Client has timed out",1);
                            callHandler(EventType::DISCONNECTED,client_map[events[i].data.fd]);
                            deleteClient(events[i].data.fd);
                            break;
                        default:
                            log("Unkown error occured while reading from client",1);
                            callHandler(EventType::DISCONNECTED,client_map[events[i].data.fd]);
                            deleteClient(events[i].data.fd);
                            break;

                    }
                    continue;
                }
                size_t safe_len = std::min<size_t>(n,MAX_MSG_LEN-1);
                buffer[safe_len] = 0;
                report.push_back(Message(buffer,client_map[events[i].data.fd]));
                callHandler(EventType::MESSAGE,client_map[events[i].data.fd],report.back());
                log(buffer,2);

            }
            if (events[i].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) {
                callHandler(EventType::DISCONNECTED,client_map[events[i].data.fd]);
                log("Client disconnected.",1);
                deleteClient(events[i].data.fd);
            }
        }
    }
    return report;
}

void MPlexServer::Server::setOnConnect(EventHandler handler) {
    handlers.onConnect = handler;
}

void MPlexServer::Server::setOnDisconnect(EventHandler handler) {
    handlers.onDisconnect = handler;
}

void MPlexServer::Server::deleteClient(int fd) {
    client_map.erase(fd);
    clientCount--;
    if (epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,nullptr) == -1) {
        log("Critical error could not delete fd from epoll.",0);
    }
    close(fd);
}

void MPlexServer::Server::callHandler(EventType event, Client client, Message msg) const {
    switch (event) {
        case EventType::CONNECTED:
            if (handlers.onConnect)
                handlers.onConnect(client);
            break;
        case EventType::DISCONNECTED:
            if (handlers.onDisconnect)
                handlers.onDisconnect(client);
            break;
        case EventType::MESSAGE:
            if (handlers.onMessage)
                handlers.onMessage(msg);
    }
}

void MPlexServer::Server::setOnMessage(MessageHandler handler) {
    handlers.onMessage = handler;
}
