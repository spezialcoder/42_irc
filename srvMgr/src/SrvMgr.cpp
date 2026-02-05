#include <vector>

#include "../../server/include/mplexserver.h"
#include "../include/IRC_macros.h"
#include "../include/SrvMgr.h"
#include "../include/utils.h"

using std::cout;
using std::endl;
using std::string;

SrvMgr::SrvMgr(MPlexServer::Server& srv, string server_password, string server_name) : srv_instance_(srv), server_password_(server_password), server_name_(server_name) {
    // server_nicks.emplace("user");
}

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
    int                     command;

    msg_parts = process_message(msg.getMessage());
    command = get_msg_type(msg_parts[0]);
    if(command == cmdType::PASS) {
        process_password(msg_parts[1], client);
    }
    
    for (auto s : msg_parts) {
        cout << "message parts '" << s << "'" << endl;
    }
    cout << "command: " << command << endl;

    if (!server_users_[client.getFd()].is_authenticated()) {
        return ;
    }
    switch (command) {
        case cmdType::CAP:
            process_cap(msg_parts[1], client);
            break;
        case cmdType::NICK:
            process_nick(msg_parts[1], client);
            break;
        case cmdType::USER:
            process_user(msg_parts[1], client);
            break;
        case cmdType::JOIN:
            break;
        case cmdType::PRIVMSG:
            break;
        case cmdType::NOTICE:
            break;
        case cmdType::MODE:
            break;
        case cmdType::INVITE:
            break;
        case cmdType::KICK:
            break;
        case cmdType::PING:
            pong(msg_parts[1], client);
            break;
        default:
            cout << "no cmd_type found.\n";
    }
}


void    SrvMgr::process_password(string provided_password, MPlexServer::Client client) {    
    // cout << "provided_password: " << provided_password << endl;
    
    if (provided_password == server_password_) {
        server_users_[client.getFd()].set_authentication(true);
    }
}

void    SrvMgr::process_cap(string s, MPlexServer::Client client) {
    if (s == "END") {
        string nick = server_users_[client.getFd()].get_nickname();
        // Send welcome messages (001-004)
        srv_instance_.sendTo(client, ":" + nick + " " + RPL_WELCOME + " :Welcome to our one server IRC 'network'.\r\n"); // wrong format, so it does not get displayed
        srv_instance_.sendTo(client, ":" + nick + " " + RPL_YOURHOST + " :Your host is " + server_name_ + ", running version 1.0.\r\n");
        srv_instance_.sendTo(client, ":" + nick + " " + RPL_CREATED + " :This server was created today.\r\n");
        srv_instance_.sendTo(client, ":" + nick + " " + RPL_MYINFO + " :server 1.0 o o\r\n");
    }
    else {
        srv_instance_.sendTo(client, "CAP * LS :\r\n");
    }
}

void    SrvMgr::process_nick(string s, MPlexServer::Client client) {
    if (server_nicks.find(s) == server_nicks.end()) {
        server_users_[client.getFd()].set_nickname(s);
    }
    else {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_NICKNAMEINUSE + " * " + s + ":Nickname already exists\r\n");
    }
}

void    SrvMgr::process_user(string s, MPlexServer::Client client) {
    int     idx = s.find_first_of(' ');
    string  username = s.substr(0, idx);
    server_users_[client.getFd()].set_username(username);

    cout << "process_user: username: " << username << endl;
}

void    SrvMgr::pong(string s, MPlexServer::Client client) {
    string nick = server_users_[client.getFd()].get_nickname();
    srv_instance_.sendTo(client, ":" + server_name_ + " PONG " + server_name_ + " " + s + "\r\n");
    cout << ":" + server_name_ + " PONG " + server_name_ + " :" + s << endl;
}