#include <vector>

#include "../../server/include/mplexserver.h"
#include "../include/SrvMgr.h"
#include "../include/utils.h"

using std::cout;
using std::endl;
using std::string;

SrvMgr::SrvMgr(MPlexServer::Server& srv, string server_password) : srv_instance_(srv), server_password_(server_password) {}

void    SrvMgr::onConnect(MPlexServer::Client client) {
    cout << "[CONNECT] New client: " << client.getIpv4() << ":" << client.getPort() << endl;
    server_users_.emplace(client.getFd(), User(client));
}

void    SrvMgr::onDisconnect(MPlexServer::Client client) {
    //std::string nick = getClientNickname(client);
    //cout << "[DISCONNECT] " << nick << " (" << client.getIpv4() << ":" << client.getPort() << ") left" << endl;
    cout << "[DISCONNECT] " << " (" << client.getIpv4() << ":" << client.getPort() << ") left" << endl;
    server_users_.erase(client.getFd());
    // Broadcast QUIT message in IRC format
    //srv_instance_.broadcast(":" + nick + " QUIT :Client disconnected\r\n");
    srv_instance_.broadcast(": QUIT :Client disconnected\r\n");
    
    // Remove nickname from maps
    //if (!nick.empty() && nick != "Unknown") {
    //    nicknameMap.erase(nick);  // erase using nickname as key, not fd
    //}
    //fdToNicknameMap.erase(client.getFd());
    //awaitingPassword.erase(client.getFd());
    //usernameMap.erase(client.getFd());
    //realnameMap.erase(client.getFd());
    //registeredClients.erase(client.getFd());
}

void    SrvMgr::onMessage(MPlexServer::Message msg) {
    MPlexServer::Client     client = msg.getClient();
    std::vector<string>     msg_parts;
    int                     msg_type;

    msg_parts = process_message(msg.getMessage());
    msg_type = get_msg_type(msg_parts[0]);
    if(msg_type == MsgType::PASS) {
        process_password(msg_parts[1], client);
    }
    
    for (auto s : msg_parts) {
        cout << "message parts '" << s << "'" << endl;
    }

    if (!server_users_[client.getFd()].is_authenticated()) {
        return ;
    }
    switch (msg_type) {
        case MsgType::CAP:
            process_cap(msg_parts[1], client);
            break;
        case MsgType::NICK:
            process_nick(msg_parts[1], client);
            break;
        case MsgType::USER:
            process_user(msg_parts[1], client);
            break;
        case MsgType::JOIN:
            break;
        case MsgType::PING:
            break;
        default:
            cout << "no message type found.\n";
    }
    
}


void    SrvMgr::process_password(string provided_password, MPlexServer::Client client) {    
    cout << provided_password << endl;
    
    if (provided_password == server_password_) {
        server_users_[client.getFd()].set_authentication(true);
        
        // Send welcome messages (001-004)
        srv_instance_.sendTo(client, ":server 001 :Welcome to the IRC Network\r\n");
        srv_instance_.sendTo(client, ":server 002 :Your host is server, running version 1.0\r\n");
        srv_instance_.sendTo(client, ":server 003 :This server was created today\r\n");
        srv_instance_.sendTo(client, ":server 004 :server 1.0 o o\r\n");
    }
}

void    SrvMgr::process_cap(string s, MPlexServer::Client client) {
    (void) s;
    srv_instance_.sendTo(client, "CAP * LS :\r\n");
}

void    SrvMgr::process_nick(string s, MPlexServer::Client client) {
    if (server_nicks.find(s) == server_nicks.end()) {
        server_users_[client.getFd()].set_nickname(s);
    }
    else {
        ;
        // disconnect
    }
}

void    SrvMgr::process_user(string s, MPlexServer::Client client) {
    server_users_[client.getFd()].set_username(s);
}
