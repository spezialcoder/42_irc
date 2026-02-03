#include <vector>

#include "../../server/include/mplexserver.h"
#include "../include/SrvMgr.h"
#include "../include/utils.h"

using std::cout;
using std::endl;
using std::string;

SrvMgr::SrvMgr(MPlexServer::Server& srv) : srv_instance_(srv) {}

void    SrvMgr::onConnect(MPlexServer::Client client) {
    User    user(client);

    cout << "[CONNECT] New client: " << client.getIpv4() << ":" << client.getPort() << endl;
        
    // Assign generic nickname -- is not part of the standard, but might make life easier.
    //std::string genericNick = generateGenericNickname();
    //setClientNickname(client, genericNick);
    
    // Mark client as awaiting password authentication
    //awaitingPassword[client.getFd()] = true;
}

void    SrvMgr::onDisconnect(MPlexServer::Client client) {
    //std::string nick = getClientNickname(client);
    //cout << "[DISCONNECT] " << nick << " (" << client.getIpv4() << ":" << client.getPort() << ") left" << endl;
    cout << "[DISCONNECT] " << " (" << client.getIpv4() << ":" << client.getPort() << ") left" << endl;
    
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
    // int fd = msg.getClient().getFd();
    MPlexServer::Client     client = msg.getClient();
    std::vector<string>     msg_parts;

    msg_parts = process_message(msg.getMessage());

    for (auto s : msg_parts) {
        cout << "message parts '" << s << "'" << endl;
    }

    switch (get_msg_type(msg_parts[0])) {
        case PASS:
            process_password(msg_parts[1], client);
            break;
        case CAP:
            break;
        case NICK:
            // process_nick();
            break;
        case USER:
            break;
        default:
            cout << "no message type found.\n";
    }
    
}

void    SrvMgr::add_user(User user) {
    all_server_users_.push_back(user);
}

void    SrvMgr::process_password(string provided_password, MPlexServer::Client client) {    
    cout << provided_password << endl;

    // password should be a member of srvMgr!!
    // if (provided_password == srv_instance_.password)

    // Send welcome messages (001-004)
    srv_instance_.sendTo(client, ":server 001 :Welcome to the IRC Network\r\n");
    srv_instance_.sendTo(client, ":server 002 :Your host is server, running version 1.0\r\n");
    srv_instance_.sendTo(client, ":server 003 :This server was created today\r\n");
    srv_instance_.sendTo(client, ":server 004 :server 1.0 o o\r\n");
}
