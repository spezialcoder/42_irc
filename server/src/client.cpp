#include "../include/mplexserver.h"

MPlexServer::Client::Client() {
    this->fd = -1;
}

MPlexServer::Client::Client(const int fd, const sockaddr_in addr) : fd(fd), client_addr(addr) {
}

MPlexServer::Client::~Client() {
}

MPlexServer::Client::Client(const Client &other) : fd(other.fd), client_addr(other.client_addr) {
}

MPlexServer::Client & MPlexServer::Client::operator=(const Client &other) {
    this->fd = other.fd;
    this->client_addr = other.client_addr;
    return *this;
}
