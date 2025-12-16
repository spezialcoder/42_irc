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
    UserManager(Server& srv) : srv_instance(srv) {}

    void onConnect(Client client) override {
        std::cout << "[CONNECT] New client: " << client.getIpv4() << ":" << client.getPort() << std::endl;
        srv_instance.sendTo(client,"Welcome to IRC Server! Type /help for commands.\r\n");
    }

    void onDisconnect(Client client) override {
        std::cout << "[DISCONNECT] Client left: " << client.getIpv4() << ":" << client.getPort() << std::endl;
    }

    void onMessage(Message msg) override {
        ParsedMessage parsed = MessageParser::parse(msg.getMessage());
        
        std::cout << "[MESSAGE RECEIVED] Type: " << MessageParser::typeToString(parsed.type) 
                  << " | From: " << msg.getClient().getIpv4() << ":" << msg.getClient().getPort()
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
            case MessageType::BROADCAST:
                srv_instance.broadcast("[" + msg.getClient().getIpv4() + "] " + msg.getMessage() + "\r\n");
                break;
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
            srv_instance.sendTo(client, "Users command not yet implemented.\r\n");
        } else if (parsed.command == "quit") {
            srv_instance.sendTo(client, "Goodbye!\r\n");
        } else {
            srv_instance.sendTo(client, "Unknown server command: /" + parsed.command + "\r\n");
        }
    }
    
    void handleChatCommand(const Client& client, const ParsedMessage& parsed) {
        if (parsed.command == "nick") {
            srv_instance.sendTo(client, "Nickname change to '" + parsed.content + "' (not yet implemented)\r\n");
        } else {
            srv_instance.sendTo(client, "Chat command /" + parsed.command + " not yet implemented.\r\n");
        }
    }
    
    void handlePrivateMessage(const Client& client, const ParsedMessage& parsed) {
        srv_instance.sendTo(client, "Private messaging not yet fully implemented. Target: " + parsed.target + "\r\n");
    }

    std::vector<Message> messages;
    Server& srv_instance;
};

int main() {
    Server srv(PORT);
    UserManager um(srv);
    srv.setEventHandler(&um);
    
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