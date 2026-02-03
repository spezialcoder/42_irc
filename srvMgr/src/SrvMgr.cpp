#include "../../server/include/mplexserver.h"
#include "../include/SrvMgr.h"
#include "../include/utils.h"

using std::cout;
using std::endl;

UsrMgnt::UsrMgnt(MPlexServer::Server& srv) : srv_instance(srv) {}

void    UsrMgnt::onConnect(MPlexServer::Client client) {
    cout << "[CONNECT] New client: " << client.getIpv4() << ":" << client.getPort() << endl;
        
    // Assign generic nickname -- is not part of the standard, but might make life easier.
    //std::string genericNick = generateGenericNickname();
    //setClientNickname(client, genericNick);
    
    // Mark client as awaiting password authentication
    //awaitingPassword[client.getFd()] = true;
}

void    UsrMgnt::onDisconnect(MPlexServer::Client client) {
    //std::string nick = getClientNickname(client);
    //cout << "[DISCONNECT] " << nick << " (" << client.getIpv4() << ":" << client.getPort() << ") left" << endl;
    cout << "[DISCONNECT] " << " (" << client.getIpv4() << ":" << client.getPort() << ") left" << endl;
    
    // Broadcast QUIT message in IRC format
    //srv_instance.broadcast(":" + nick + " QUIT :Client disconnected\r\n");
    srv_instance.broadcast(": QUIT :Client disconnected\r\n");
    
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

void    UsrMgnt::onMessage(MPlexServer::Message msg) {
    // int fd = msg.getClient().getFd();
    std::string rawMsg = msg.getMessage();
    strip_trailing_rn(rawMsg);

    cout << "[DEBUG] " << rawMsg << endl;
    // Send welcome messages (001-004)
    srv_instance.sendTo(msg.getClient(), ":server 001 :Welcome to the IRC Network\r\n");
    srv_instance.sendTo(msg.getClient(), ":server 002 :Your host is server, running version 1.0\r\n");
    srv_instance.sendTo(msg.getClient(), ":server 003 :This server was created today\r\n");
    srv_instance.sendTo(msg.getClient(), ":server 004 :server 1.0 o o\r\n");

}
