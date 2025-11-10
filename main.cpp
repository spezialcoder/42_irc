#include <iostream>
#include "server/include/mplexserver.h"

int main() {
    MPlexServer::MPlexServer server(785);
    server.setVerbose(2);
    server.activate();
    while (true) {
        server.poll();
        std::cout << "Poll" << std::endl;
        sleep(2);
    }
    return 0;
}