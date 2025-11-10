#include "../include/mplexserver.h"

MPlexServer::Client::Client(const int fd, const sockaddr_in addr) : fd(fd), client_addr(addr) {
}

MPlexServer::Client::~Client() {
}