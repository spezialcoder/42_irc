#include <iostream>
#include "server/include/mplexserver.h"
#include <vector>

using namespace MPlexServer;

constexpr int PORT=7850;

class UserManager : public EventHandler {
public:
    void onConnect(Client client) override {
        std::cout << "New client: " << client.getIpv4() << ", " << client.getPort() << std::endl;
    }

    void onDisconnect(Client client) override {
        std::cout << "Client left: " << client.getIpv4() << ", " << client.getPort() << std::endl;

    }

    void onMessage(Message msg) override {
        std::cout << "New message: " << msg.getMessage() << std::endl;
        messages.emplace_back(msg);
    }
private:
    std::vector<Message> messages;
};

int main() {
    Server srv(PORT);
    UserManager um;
    srv.setEventHandler(&um);
    try {
        srv.activate();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    while (true) {
        srv.poll();
    }

    return 0;
}