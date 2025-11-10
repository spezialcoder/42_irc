#include <iostream>
#include "server/include/mplexserver.h"
using namespace MPlexServer;

int main() {
    Server server(785);
    server.setVerbose(2);
    server.activate();

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