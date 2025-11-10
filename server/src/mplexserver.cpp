#include "../include/mplexserver.h"

MPlexServer::MPlexServer::MPlexServer(uint16_t port, const std::string ipv4) : port(port), ipv4(ipv4) {
    this->verbose = 0;
    this->server_fd = -1;
    this->epollfd = -1;
    this->clientCount = 0;
}

MPlexServer::MPlexServer::~MPlexServer() {
}

void MPlexServer::MPlexServer::activate() {
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
    ev.events = EPOLLIN | EPOLLRDHUP;
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

void MPlexServer::MPlexServer::log(const std::string message, const int required_level) const {
    if (this->verbose >= required_level) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm = *std::localtime(&now_time);
        std::cout << "[MPlexServer][" << std::put_time(&local_tm, "%Y-%m-%d@%H:%M:%S") << "] " << message << std::endl;
    }
}

void MPlexServer::MPlexServer::deactivate() {
    //TODO: Proper closing
    if (server_fd != -1) close(server_fd);
    if (epollfd != -1) close(epollfd);
}

void MPlexServer::MPlexServer::setVerbose(const int level) {
    if (level <= VERBOSITY_MAX && level >= 0) {
        this->verbose = level;
    } else {
        throw ServerSettingsError("Verbosity level does not exist");
    }
}

int MPlexServer::MPlexServer::getVerbose() const {
    return this->verbose;
}

int MPlexServer::MPlexServer::getConnectedClientsCount() const {
    return 0;
}

std::vector<MPlexServer::Message> MPlexServer::MPlexServer::poll() {
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
            int clientFd = accept(server_fd, (struct sockaddr*)&client_addr,&len);
            if (clientFd == -1) {
                log("Failed to accept client.",0);
                continue;
            }
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
        } else {
            if (events[i].events & EPOLLIN) {
                char buffer[MAX_MSG_LEN];
                ssize_t n = recv(events[i].data.fd, buffer, MAX_MSG_LEN,MSG_DONTWAIT);
                if (n==0) {
                    log("Client disconnected.",1);
                    //TODO: Disconnect client
                    deleteClient(events[i].data.fd);
                    continue;
                }
                size_t safe_len = std::min<size_t>(n,MAX_MSG_LEN-1);
                buffer[safe_len] = 0;
                report.push_back(Message(buffer,client_map[events[i].data.fd]));

            }
            if (events[i].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) {
                //TODO: Do it nicely
                log("Client disconnected.",1);
                deleteClient(events[i].data.fd);
            }
        }
    }
    return report;
}

void MPlexServer::MPlexServer::deleteClient(int fd) {
    client_map.erase(fd);
    clientCount--;
    if (epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,nullptr) == -1) {
        log("Critical error could not delete fd from epoll.",0);
    }
    close(fd);
}
