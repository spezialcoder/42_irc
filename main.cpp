#include <iostream>
#include <chrono>
#include <sstream>
#include "server/include/mplexserver.h"
#include <vector>
#include <map>

using namespace MPlexServer;

constexpr int PORT=7850;

enum class MessageType {
    SERVER_COMMAND,    // /server commands (e.g., /quit, /help)
    CHAT_COMMAND,      // /chat commands (e.g., /msg, /nick)
    BROADCAST,         // Regular chat message
    PRIVATE            // Private message
};

struct ParsedMessage {
    MessageType type;
    std::string command;
    std::string target;
    std::string content;
};

class MessageParser {
public:
    static ParsedMessage parse(const std::string& msg) {
        ParsedMessage result;
        
        // Check if message starts with /
        if (msg.empty() || msg[0] != '/') {
            result.type = MessageType::BROADCAST;
            result.content = msg;
            return result;
        }
        
        // Extract command
        std::istringstream iss(msg.substr(1));
        std::string cmd;
        iss >> cmd;
        result.command = cmd;
        
        // Server commands
        if (cmd == "quit" || cmd == "help" || cmd == "users" || cmd == "server") {
            result.type = MessageType::SERVER_COMMAND;
            std::getline(iss, result.content);
            if (!result.content.empty() && result.content[0] == ' ')
                result.content = result.content.substr(1);
            return result;
        }
        
        // Chat commands
        if (cmd == "msg" || cmd == "whisper" || cmd == "pm") {
            result.type = MessageType::PRIVATE;
            iss >> result.target;
            std::getline(iss, result.content);
            if (!result.content.empty() && result.content[0] == ' ')
                result.content = result.content.substr(1);
            return result;
        }
        
        if (cmd == "nick" || cmd == "join" || cmd == "leave") {
            result.type = MessageType::CHAT_COMMAND;
            std::getline(iss, result.content);
            if (!result.content.empty() && result.content[0] == ' ')
                result.content = result.content.substr(1);
            return result;
        }
        
        // Unknown command - treat as broadcast
        result.type = MessageType::BROADCAST;
        result.content = msg;
        return result;
    }
    
    static std::string typeToString(MessageType type) {
        switch (type) {
            case MessageType::SERVER_COMMAND: return "SERVER_COMMAND";
            case MessageType::CHAT_COMMAND:   return "CHAT_COMMAND";
            case MessageType::BROADCAST:      return "BROADCAST";
            case MessageType::PRIVATE:        return "PRIVATE";
            default: return "UNKNOWN";
        }
    }
};

class UserManager : public EventHandler {
public:
    UserManager(Server& srv) : srv_instance(srv), nextGuestId(1) {}

    void onConnect(Client client) override {
        std::cout << "[CONNECT] New client: " << client.getIpv4() << ":" << client.getPort() << std::endl;
        
        // Assign generic nickname
        std::string genericNick = generateGenericNickname();
        setClientNickname(client, genericNick);
        
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
    }

    void onMessage(Message msg) override {
        ParsedMessage parsed = MessageParser::parse(msg.getMessage());
        std::string senderNick = getClientNickname(msg.getClient());
        
        std::cout << "[MESSAGE RECEIVED] Type: " << MessageParser::typeToString(parsed.type) 
                  << " | From: " << senderNick << " (" << msg.getClient().getIpv4() << ":" << msg.getClient().getPort() << ")"
                  << " | Raw: \"" << msg.getMessage() << "\"" << std::endl;
        
        messages.emplace_back(msg);
        
        // Handle different message types
        switch (parsed.type) {
            case MessageType::SERVER_COMMAND:
                handleServerCommand(msg.getClient(), parsed);
                break;
            case MessageType::CHAT_COMMAND:
                handleChatCommand(msg.getClient(), parsed);
                break;
            case MessageType::PRIVATE:
                handlePrivateMessage(msg.getClient(), parsed);
                break;
            case MessageType::BROADCAST: {
                // Strip trailing \r\n from message content
                std::string cleanMsg = msg.getMessage();
                if (cleanMsg.size() >= 2 && cleanMsg.substr(cleanMsg.size()-2) == "\r\n") {
                    cleanMsg = cleanMsg.substr(0, cleanMsg.size()-2);
                } else if (!cleanMsg.empty() && cleanMsg.back() == '\n') {
                    cleanMsg.pop_back();
                }
                // IRC protocol: :nickname PRIVMSG #channel :message
                // Don't send to sender (IRC standard - client shows own message locally)
                srv_instance.broadcastExcept(msg.getClient(), ":" + senderNick + " PRIVMSG #general :" + cleanMsg + "\r\n");
                break;
            }
        }
    }
    
private:
    void handleServerCommand(const Client& client, const ParsedMessage& parsed) {
        if (parsed.command == "help") {
            srv_instance.sendTo(client, "Available commands:\r\n"
                "/help - Show this help\r\n"
                "/users - List connected users\r\n"
                "/msg <user> <message> - Send private message\r\n"
                "/nick <name> - Set nickname\r\n"
                "/quit - Disconnect\r\n");
        } else if (parsed.command == "users") {
            listUsers(client);
        } else if (parsed.command == "quit") {
            srv_instance.sendTo(client, "Goodbye!\r\n");
        } else {
            srv_instance.sendTo(client, "Unknown server command: /" + parsed.command + "\r\n");
        }
    }
    
    void handleChatCommand(const Client& client, const ParsedMessage& parsed) {
        if (parsed.command == "nick") {
            std::string newNick = parsed.content;
            
            // Strip any trailing whitespace, \r, \n
            while (!newNick.empty() && (newNick.back() == ' ' || newNick.back() == '\t' || 
                                        newNick.back() == '\r' || newNick.back() == '\n')) {
                newNick.pop_back();
            }
            // Strip leading whitespace
            size_t start = 0;
            while (start < newNick.length() && (newNick[start] == ' ' || newNick[start] == '\t')) {
                start++;
            }
            newNick = newNick.substr(start);
            
            std::string currentNick = getClientNickname(client);
            
            // Validate nickname
            if (newNick.empty()) {
                // 431 ERR_NONICKNAMEGIVEN
                srv_instance.sendTo(client, ":server 431 " + currentNick + " :No nickname given\r\n");
                return;
            }
            
            // Check for invalid characters (spaces, special chars)
            if (newNick.find(' ') != std::string::npos || newNick.find('\t') != std::string::npos) {
                // 432 ERR_ERRONEUSNICKNAME
                srv_instance.sendTo(client, ":server 432 " + currentNick + " " + newNick + " :Erroneous nickname\r\n");
                return;
            }
            
            // Check nickname length
            if (newNick.length() > 20) {
                // 432 ERR_ERRONEUSNICKNAME
                srv_instance.sendTo(client, ":server 432 " + currentNick + " " + newNick + " :Erroneous nickname (too long)\r\n");
                return;
            }
            
            // Check if nickname is already taken
            if (isNicknameTaken(newNick) && getClientNickname(client) != newNick) {
                // 433 ERR_NICKNAMEINUSE
                srv_instance.sendTo(client, ":server 433 " + currentNick + " " + newNick + " :Nickname is already in use\r\n");
                return;
            }
            
            std::string oldNick = getClientNickname(client);
            setClientNickname(client, newNick);
            
            // IRC protocol: :oldnick NICK :newnick
            srv_instance.broadcast(":" + oldNick + " NICK :" + newNick + "\r\n");
            
            std::cout << "[NICK] " << oldNick << " changed nickname to " << newNick << std::endl;
        } else {
            srv_instance.sendTo(client, "Chat command /" + parsed.command + " not yet implemented.\r\n");
        }
    }
    
    void handlePrivateMessage(const Client& client, const ParsedMessage& parsed) {
        srv_instance.sendTo(client, "Private messaging not yet fully implemented. Target: " + parsed.target + "\r\n");
    }

private:
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
    
    // List all connected users (IRC protocol: 353 RPL_NAMREPLY, 366 RPL_ENDOFNAMES)
    void listUsers(const Client& client) {
        std::string requesterNick = getClientNickname(client);
        
        // Build user list
        std::string userList;
        for (const auto& [fd, nickname] : fdToNicknameMap) {
            if (!userList.empty()) {
                userList += " ";
            }
            userList += nickname;
        }
        
        // Send both messages as one to avoid buffer issues
        std::string response;
        // 353 RPL_NAMREPLY - Format: :server 353 nickname = #channel :user list
        response += ":server 353 " + requesterNick + " = #general :" + userList + "\r\n";
        // 366 RPL_ENDOFNAMES - Format: :server 366 nickname #channel :End of /NAMES list
        response += ":server 366 " + requesterNick + " #general :End of /NAMES list\r\n";
        
        srv_instance.sendTo(client, response);
    }

    std::vector<Message> messages;
    Server& srv_instance;
    int nextGuestId;
    std::map<std::string, int> nicknameMap;  // nickname -> fd
    std::map<int, std::string> fdToNicknameMap;  // fd -> nickname
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