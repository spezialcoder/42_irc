#include <iostream>
#include <chrono>
#include <sstream>
#include "server/include/mplexserver.h"
#include <vector>
#include <map>

using namespace MPlexServer;

constexpr int PORT=6667;

class UserManager : public EventHandler {
public:
    UserManager(Server& srv) : srv_instance(srv), nextGuestId(1) {}

    void onConnect(Client client) override {
        std::cout << "[CONNECT] New client: " << client.getIpv4() << ":" << client.getPort() << std::endl;
        
        // Assign generic nickname
        std::string genericNick = generateGenericNickname();
        setClientNickname(client, genericNick);
        
        // Mark client as awaiting password authentication
        awaitingPassword[client.getFd()] = true;
        
        // IRC protocol welcome sequence with numeric replies
        // 001 RPL_WELCOME
        srv_instance.sendTo(client, ":server 001 " + genericNick + " :Welcome to the IRC Network " + genericNick + "\r\n");
        // 002 RPL_YOURHOST
        srv_instance.sendTo(client, ":server 002 " + genericNick + " :Your host is server, running version 1.0\r\n");
        // 003 RPL_CREATED
        srv_instance.sendTo(client, ":server 003 " + genericNick + " :This server was created today\r\n");
        // 004 RPL_MYINFO
        srv_instance.sendTo(client, ":server 004 " + genericNick + " :server 1.0 o o\r\n");
    }

    void onDisconnect(Client client) override {
        std::string nick = getClientNickname(client);
        std::cout << "[DISCONNECT] " << nick << " (" << client.getIpv4() << ":" << client.getPort() << ") left" << std::endl;
        
        // Broadcast QUIT message in IRC format
        srv_instance.broadcast(":" + nick + " QUIT :Client disconnected\r\n");
        
        // Remove nickname from maps
        if (!nick.empty() && nick != "Unknown") {
            nicknameMap.erase(nick);  // erase using nickname as key, not fd
        }
        fdToNicknameMap.erase(client.getFd());
        awaitingPassword.erase(client.getFd());
        usernameMap.erase(client.getFd());
        realnameMap.erase(client.getFd());
    }

    void onMessage(Message msg) override {
        int fd = msg.getClient().getFd();
        std::string rawMsg = msg.getMessage();
        std::string senderNick = getClientNickname(msg.getClient());
        
        // Strip trailing \r\n for processing
        while (!rawMsg.empty() && (rawMsg.back() == '\r' || rawMsg.back() == '\n')) {
            rawMsg.pop_back();
        }
        
        // Handle PASS command for authentication
        if (rawMsg.substr(0, 5) == "PASS ") {
            std::string providedPassword = rawMsg.substr(5);
            // TODO: Validate password against expected password
            // For now, accept any password
            awaitingPassword[fd] = false;
            
            // Send welcome messages after authentication
            srv_instance.sendTo(msg.getClient(), ":server 001 " + senderNick + " :Welcome to the IRC Network " + senderNick + "\r\n");
            srv_instance.sendTo(msg.getClient(), ":server 002 " + senderNick + " :Your host is server, running version 1.0\r\n");
            srv_instance.sendTo(msg.getClient(), ":server 003 " + senderNick + " :This server was created today\r\n");
            srv_instance.sendTo(msg.getClient(), ":server 004 " + senderNick + " :server 1.0 o o\r\n");
            std::cout << "[AUTH] Client " << senderNick << " authenticated with PASS command" << std::endl;
            return;
        }
        
        // Reject commands if not authenticated
        if (awaitingPassword.find(fd) != awaitingPassword.end() && awaitingPassword[fd]) {
            srv_instance.sendTo(msg.getClient(), "ERROR :You must send PASS first\r\n");
            return;
        }
        
        // Parse IRC protocol messages like PRIVMSG
        if (rawMsg.substr(0, 8) == "PRIVMSG ") {
            // Format: PRIVMSG #channel :message or PRIVMSG target :message
            size_t colonPos = rawMsg.find(" :");
            if (colonPos != std::string::npos) {
                std::string target = rawMsg.substr(8, colonPos - 8);
                std::string content = rawMsg.substr(colonPos + 2);
                
                std::cout << "[PRIVMSG] From: " << senderNick << " | Target: " << target << " | Content: " << content << std::endl;
                
                // Check if target is a channel (starts with #)
                if (!target.empty() && target[0] == '#') {
                    // Broadcast to all other clients in channel
                    srv_instance.broadcastExcept(msg.getClient(), ":" + senderNick + " PRIVMSG " + target + " :" + content + "\r\n");
                } else {
                    // Private message to user
                    // TODO: Find user by nickname and send
                    srv_instance.sendTo(msg.getClient(), ":server NOTICE " + senderNick + " :Private messaging not yet implemented\r\n");
                }
                return;
            }
        }
        
        // Handle NICK command
        if (rawMsg.substr(0, 5) == "NICK ") {
            std::string newNick = rawMsg.substr(5);
            
            // Strip whitespace
            while (!newNick.empty() && (newNick.back() == ' ' || newNick.back() == '\t')) {
                newNick.pop_back();
            }
            
            std::string currentNick = getClientNickname(msg.getClient());
            
            if (newNick.empty()) {
                srv_instance.sendTo(msg.getClient(), ":server 431 " + currentNick + " :No nickname given\r\n");
                return;
            }
            
            if (!isValidNick(newNick)) {
                srv_instance.sendTo(msg.getClient(), ":server 432 " + currentNick + " " + newNick + " :Erroneous nickname\r\n");
                return;
            }
            
            if (isNicknameTaken(newNick)) {
                srv_instance.sendTo(msg.getClient(), ":server 433 " + currentNick + " " + newNick + " :Nickname is already in use\r\n");
                return;
            }
            
            setClientNickname(msg.getClient(), newNick);
            srv_instance.sendTo(msg.getClient(), ":" + currentNick + " NICK :" + newNick + "\r\n");
            srv_instance.broadcastExcept(msg.getClient(), ":" + currentNick + " NICK :" + newNick + "\r\n");
            std::cout << "[NICK] " << currentNick << " changed nickname to " << newNick << std::endl;
            return;
        }
        
        // Handle USER command (required for IRC registration)
        if (rawMsg.substr(0, 5) == "USER ") {
            // Format: USER username 0 * :Real Name
            std::istringstream iss(rawMsg.substr(5));
            std::string username, mode, unused;
            iss >> username >> mode >> unused;
            
            // Extract real name after the colon
            std::string realname;
            size_t colonPos = rawMsg.find(" :");
            if (colonPos != std::string::npos) {
                realname = rawMsg.substr(colonPos + 2);
            } else {
                realname = username;  // Fallback to username if no realname provided
            }
            
            // Store username and realname
            usernameMap[fd] = username;
            realnameMap[fd] = realname;
            
            std::cout << "[USER] Client " << senderNick << " set username: " << username 
                      << ", realname: " << realname << std::endl;
            return;
        }
        
        // Handle PING command (keepalive)
        if (rawMsg.substr(0, 5) == "PING ") {
            std::string token = rawMsg.substr(5);
            srv_instance.sendTo(msg.getClient(), "PONG server :" + token + "\r\n");
            return;
        }
        
        if (rawMsg == "PING") {
            srv_instance.sendTo(msg.getClient(), "PONG server\r\n");
            return;
        }
        
        // Handle CAP command (capability negotiation - just end it)
        if (rawMsg.substr(0, 4) == "CAP ") {
            std::string capCmd = rawMsg.substr(4);
            if (capCmd.substr(0, 2) == "LS") {
                // List capabilities - we support none, so send empty
                srv_instance.sendTo(msg.getClient(), ":server CAP * LS :\r\n");
            } else if (capCmd.substr(0, 3) == "END") {
                // Client ending capability negotiation
                std::cout << "[CAP] Client " << senderNick << " ended capability negotiation" << std::endl;
            }
            return;
        }
        
        // Handle QUIT command
        if (rawMsg.substr(0, 4) == "QUIT") {
            std::string quitMsg = "Client quit";
            size_t colonPos = rawMsg.find(" :");
            if (colonPos != std::string::npos) {
                quitMsg = rawMsg.substr(colonPos + 2);
            }
            srv_instance.sendTo(msg.getClient(), ":server NOTICE " + senderNick + " :Goodbye!\r\n");
            srv_instance.disconnectClient(msg.getClient());
            return;
        }
        
        // Handle WHO command (list users)
        if (rawMsg == "WHO" || rawMsg.substr(0, 4) == "WHO ") {
            // 352 RPL_WHOREPLY format: <channel> <user> <host> <server> <nick> <H|G> :<hopcount> <real name>
            for (const auto& [clientFd, nickname] : fdToNicknameMap) {
                // Get username and realname if set, otherwise use nickname as fallback
                std::string username = (usernameMap.find(clientFd) != usernameMap.end()) 
                                       ? usernameMap.at(clientFd) : nickname;
                std::string realname = (realnameMap.find(clientFd) != realnameMap.end()) 
                                       ? realnameMap.at(clientFd) : nickname;
                
                srv_instance.sendTo(msg.getClient(), ":server 352 " + senderNick + " * " 
                                   + username + " localhost server " + nickname 
                                   + " H :0 " + realname + "\r\n");
            }
            // 315 RPL_ENDOFWHO
            srv_instance.sendTo(msg.getClient(), ":server 315 " + senderNick + " * :End of WHO list\r\n");
            return;
        }
        
        // Unknown command
        std::string cmd = rawMsg.substr(0, rawMsg.find(' '));
        srv_instance.sendTo(msg.getClient(), ":server 421 " + senderNick + " " + cmd + " :Unknown command\r\n");
    }
    
private:
    // Validate nickname according to IRC standards
    bool isValidNick(const std::string& nick) const {
        if (nick.empty() || nick.size() > 32) return false;
        static const std::string bad = " ,*?!@:.";
        for (char c : nick) {
            if (bad.find(c) != std::string::npos) return false;
        }
        return true;
    }
    
    // Generate a generic nickname for new clients
    std::string generateGenericNickname() {
        std::string nick = "Guest" + std::to_string(nextGuestId++);
        // Ensure uniqueness (in case of overflow/wraparound)
        while (isNicknameTaken(nick)) {
            nick = "Guest" + std::to_string(nextGuestId++);
        }
        return nick;
    }
    
    // Check if a nickname is already in use
    bool isNicknameTaken(const std::string& nick) const {
        for (const auto& [fd, nickname] : fdToNicknameMap) {
            if (nickname == nick) {
                return true;
            }
        }
        return false;
    }
    
    // Set nickname for a client
    void setClientNickname(const Client& client, const std::string& nick) {
        int fd = client.getFd();
        
        // Remove old nickname from reverse map if exists
        if (fdToNicknameMap.find(fd) != fdToNicknameMap.end()) {
            nicknameMap.erase(fdToNicknameMap[fd]);
        }
        
        // Set new nickname
        fdToNicknameMap[fd] = nick;
        nicknameMap[nick] = fd;
    }
    
    // Get nickname for a client
    std::string getClientNickname(const Client& client) const {
        int fd = client.getFd();
        auto it = fdToNicknameMap.find(fd);
        if (it != fdToNicknameMap.end()) {
            return it->second;
        }
        return "Unknown";
    }

    std::vector<Message> messages;
    Server& srv_instance;
    int nextGuestId;
    std::map<std::string, int> nicknameMap;  // nickname -> fd
    std::map<int, std::string> fdToNicknameMap;  // fd -> nickname
    std::map<int, bool> awaitingPassword;  // fd -> awaiting password auth
    std::map<int, std::string> usernameMap;  // fd -> username (for USER command)
    std::map<int, std::string> realnameMap;  // fd -> realname (for USER command)
};

int main() {
    Server srv(PORT);
    UserManager um(srv);
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