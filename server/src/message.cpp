#include "../include/mplexserver.h"

MPlexServer::Message::Message() {}

MPlexServer::Message::Message(std::string msg, Client client) {
    this->message = msg;
    this->client = client;
}

MPlexServer::Message::Message(const Message &other) {
    this->message = other.message;
    this->client = other.client;
}

const MPlexServer::Client & MPlexServer::Message::getClient() const {
    return this->client;
}

std::string MPlexServer::Message::getMessage() const {
    return this->message;
}

void MPlexServer::Message::setMessage(std::string msg) {
    this->message = msg;
}

void MPlexServer::Message::setClient(Client &client) {
    this->client = client;
}

MPlexServer::Message & MPlexServer::Message::operator=(const Message &other) {
    this->message = other.message;
    this->client = other.client;
    return *this;
}