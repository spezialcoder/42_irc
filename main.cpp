#include <iostream>
#include "server/include/mplexserver.h"
using namespace MPlexServer;

void myOnConnect(Client client) {
    std::cout << "New client connected: " << client.getIpv4() << ", " << client.getPort() << std::endl;
}

int main() {
    Server server(785);
    server.setVerbose(0);
    server.activate();
    server.setOnConnect(myOnConnect);
    while (true) {
        std::vector<Message> messages = server.poll();
        sleep(1);
        std::cout << "{";
        for (const auto& message : messages) {
            std::cout << message.getMessage() << ",";
        }
        std::cout << "}" << std::endl;
    }
    return 0;
}