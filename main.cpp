#include <iostream>
#include "server/include/mplexserver.h"
#include <vector>

using namespace MPlexServer;

constexpr int PORT=7850;

class UserManager : public EventHandler {
public:
    UserManager(Server& srv) : srv_instance(srv) {}

    void onConnect(Client client) override {
        std::cout << "New client: " << client.getIpv4() << ", " << client.getPort() << std::endl;
        srv_instance.sendTo(client,"Hello there client.");
    }

    void onDisconnect(Client client) override {
        std::cout << "Client left: " << client.getIpv4() << ", " << client.getPort() << std::endl;

    }

    void onMessage(Message msg) override {
        std::cout << "New message: " << msg.getMessage() << std::endl;
        messages.emplace_back(msg);
        //srv_instance.sendTo(msg.getClient(),"Echo: "+msg.getMessage());
        srv_instance.broadcast(msg.getMessage());
    }
private:
    std::vector<Message> messages;
    Server& srv_instance;
};

int main() {
    Server srv(PORT);
    UserManager um(srv);
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