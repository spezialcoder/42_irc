#include <iostream>
#include "server/include/mplexserver.h"
using namespace MPlexServer;

constexpr int PORT=785;

void myOnConnect(Client client) {
    std::cout << "New client connected: " << client.getIpv4() << ", " << client.getPort() << std::endl;
}

void myOnMessage(Message msg) {
    std::cout << "New message: " << msg.getMessage();
}

int main() {
    Server server(PORT);
    server.setVerbose(0);
    server.activate();
    server.setOnConnect(myOnConnect);
    server.setOnMessage(myOnMessage);
    while (true) {
        std::vector<Message> messages = server.poll();
    }
    return 0;
}