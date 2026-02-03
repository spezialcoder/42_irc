#include <iostream>
#include <chrono>
#include <sstream>
#include <vector>
#include <map>
#include <set>

#include "server/include/mplexserver.h"
#include "srvMgr/include/UsrMgnt.h"
#include "srvMgr/include/utils.h"

using namespace MPlexServer;

constexpr int PORT=6667;

int main() {
    Server  srv(PORT);
    //UserManager um(srv);
    UsrMgnt um(srv);
    srv.setEventHandler(&um);
    srv.setVerbose(1);  // 1: Reduce logging - only important messages 2: Debug info - verbose
    
    try {
        srv.activate();
        std::cout << "[SERVER] Started on port " << PORT << std::endl;
    } catch (std::exception &e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
    
    // Heartbeat tracking
    auto server_start = std::chrono::steady_clock::now();
    auto last_heartbeat = std::chrono::steady_clock::now();
    const auto heartbeat_interval = std::chrono::seconds(10);
    
    std::cout << "[SERVER] Alive - waiting for connections..." << std::endl;
    
    while (true) {
        srv.poll();
        
        // Check if 10 seconds have passed for heartbeat
        auto now = std::chrono::steady_clock::now();
        if (now - last_heartbeat >= heartbeat_interval) {
            auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - server_start).count();
            std::cout << "[SERVER] Alive - Uptime: " << uptime << "s" << std::endl;
            last_heartbeat = now;
        }
    }

    return 0;
}