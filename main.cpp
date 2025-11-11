#include <iostream>
#include "server/include/mplexserver.h"
#include <vector>

using namespace MPlexServer;

constexpr int PORT=7850;


class User {
public:
    User() = default;

    static void onNewMessage(Message msg, User* user) {
        std::cout << "New message: " << msg.getMessage() << std::endl;
        user->messages.push_back(msg);
    }
private:
    std::vector<Message> messages;
};

int main() {
    Server srv(PORT);
    User userbase;
    srv.setVerbose(2);
    srv.setOnMessage(User::onNewMessage, &userbase);
    try {
        srv.activate();
    } catch (ServerError &e) {}

    while (true) {
        srv.poll();
    }

    return 0;
}