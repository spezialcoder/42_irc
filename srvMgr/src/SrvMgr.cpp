#include <vector>

#include "../../server/include/mplexserver.h"
#include "../include/IRC_macros.h"
#include "../include/SrvMgr.h"
#include "../include/utils.h"

using std::cout;
using std::endl;
using std::string;

SrvMgr::SrvMgr(MPlexServer::Server& srv, const string& server_password, const string& server_name) : srv_instance_(srv), server_password_(server_password), server_name_(server_name) {
    // server_nicks.emplace("user");
    server_nicks.emplace("dnlspr");
}

void    SrvMgr::onConnect(MPlexServer::Client client) {
    cout << "[CONNECT] New client: " << client.getIpv4() << ":" << client.getPort() << endl;
    server_users_.emplace(client.getFd(), User(client));
}

void    SrvMgr::onDisconnect(MPlexServer::Client client) {
    User&   user = server_users_[client.getFd()];
    std::string nick = user.get_nickname();
    cout << "[DISCONNECT] " << nick << " (" << client.getIpv4() << ":" << client.getPort() << ") left" << endl;
    server_users_.erase(client.getFd());
    server_nicks.erase(nick);

    // Broadcast QUIT message in IRC format
    srv_instance_.broadcast(":" + nick + " QUIT :Client disconnected\r\n");
    
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

void    SrvMgr::onMessage(const MPlexServer::Message msg) {
    const MPlexServer::Client&  client = msg.getClient();
    User&                       user = server_users_[client.getFd()];
    std::vector<string>         msg_parts = process_message(msg.getMessage());
    const int                   command = get_msg_type(msg_parts[0]);


    for (auto s : msg_parts) {
        cout << "message parts '" << s << "'" << endl;
    }
    cout << "command: " << command << endl;

    switch (command) {
        case cmdType::PASS:
            process_password(msg_parts[1], client, user);
            break;
        case cmdType::CAP:
            process_cap(msg_parts[1], client, user);
            break;
        case cmdType::NICK:
            process_nick(msg_parts[1], client, user);
            break;
        case cmdType::USER:
            process_user(msg_parts[1], client, user);
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
        case cmdType::QUIT:
            break;
        case cmdType::PING:
            pong(msg_parts[1], client, user);
            break;
        default:
            cout << "no cmd_type found.\n";
    }

}

void    SrvMgr::try_to_log_in(User &user, const MPlexServer::Client &client) const {
    if (user.get_nickname().empty() || user.get_username().empty() ||
        !user.password_provided() || !user.cap_negotiation_ended()) {
        return ;
    }
    user.set_as_logged_in(true);
    // Send welcome messages (001-004)
    const string nick = user.get_nickname();
    srv_instance_.sendTo(client, ":" + nick + " " + RPL_WELCOME + " :Welcome to our single-server IRC 'network'.\r\n"); // wrong format, so it does not get displayed
    srv_instance_.sendTo(client, ":" + nick + " " + RPL_YOURHOST + " :Your host is " + server_name_ + ", running version 1.0.\r\n");
    srv_instance_.sendTo(client, ":" + nick + " " + RPL_CREATED + " :This server was created today.\r\n");
    srv_instance_.sendTo(client, ":" + nick + " " + RPL_MYINFO + " :server 1.0 o o\r\n");
}


void    SrvMgr::process_password(const std::string& provided_password, const MPlexServer::Client& client, User& user) const {
    if (user.is_logged_in()) {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_ALREADYREGISTERED + " " + user.get_nickname() + " " + ":You may not reregister\r\n");
        return ;
    }
    if (provided_password == server_password_) {
        user.set_password_provided(true);
    }
    else {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_PASSWDMISMATCH + " * " + ":Password incorrect\r\n");
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_NOTREGISTERED + " * " + ":You have not registered\r\n");
        srv_instance_.sendTo(client, "ERROR :Closing Link: " + client.getIpv4() + " (Password incorrect)\r\n");
        srv_instance_.disconnectClient(client);
        return ;
    }
    if (!user.is_logged_in()) {
        try_to_log_in(user ,client);
    }
}

void    SrvMgr::process_cap(const string& s, const MPlexServer::Client& client, User& user) const {
    if (s == "END") {
        user.set_cap_negotiation_ended(true);
    }
    else {
        srv_instance_.sendTo(client, "CAP * LS :\r\n");
    }
    if (!user.is_logged_in()) {
        try_to_log_in(user ,client);
    }
}

void    SrvMgr::process_nick(const string& s, const MPlexServer::Client& client, User& user) {
    if (s.find_first_of("#:; ") != s.npos) {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_ERRONEUSNICKNAME + " " + user.get_nickname() + " " + s + ":Erroneus nickname\r\n");
        return ;
    }
    if (server_nicks.find(s) == server_nicks.end()) {
        if (!(user.get_nickname() == "*")) {
            server_nicks.erase(user.get_nickname());
        }
        server_nicks.emplace(s);
        user.set_nickname(s);
    }
    else {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_NICKNAMEINUSE + " " + user.get_nickname() + " " + s + ":Nickname is already in use\r\n");
    }
    if (!user.is_logged_in()) {
        try_to_log_in(user ,client);
    }
}

void    SrvMgr::process_user(string s, const MPlexServer::Client& client, User& user) const {
    size_t     idx;

    if (user.is_logged_in()) {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_ALREADYREGISTERED + " " + user.get_nickname() + " " + ":You may not reregister\r\n");
        return ;
    }

    idx = s.find_first_of(' ');
    string  username = s.substr(0, idx);
    s = s.substr(idx + 1, s.length() - idx);
    idx = s.find_first_of(' ');
    string  hostname = s.substr(0, idx);

    if (username.empty() || hostname.empty()) {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_NEEDMOREPARAMS + " * " + ":Not enough parameters for user registration\r\n");
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_NOTREGISTERED + " * " + ":You have not registered\r\n");
    }

    user.set_username(username);
    user.set_username(hostname);

    cout << "process_user: username: " << username
        << " ,hostname: " << hostname << endl;
    if (!user.is_logged_in()) {
        try_to_log_in(user ,client);
    }

    // after CAP, NICK and USER
    if (!user.password_provided()) {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_PASSWDMISMATCH + " * " + ":Password incorrect\r\n");
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_NOTREGISTERED + " * " + ":You have not registered\r\n");
        srv_instance_.sendTo(client, "ERROR :Closing Link: " + client.getIpv4() + " (Password incorrect)\r\n");
        srv_instance_.disconnectClient(client);
    }
}

void    SrvMgr::pong(const string &s, const MPlexServer::Client &client, const User&) {
    string nick = server_users_[client.getFd()].get_nickname();
    srv_instance_.sendTo(client, ":" + server_name_ + " PONG " + server_name_ + " " + s + "\r\n");
    cout << ":" + server_name_ + " PONG " + server_name_ + " :" + s << endl;
}