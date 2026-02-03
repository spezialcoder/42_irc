#pragma once

#include <string>
#include <vector>

#include "./mplexserver.h"

class Channel;

/**
 * @brief Class to manage users and their capabilities
*/
class SrvMgr : public MPlexServer::EventHandler {
public:
    SrvMgr(MPlexServer::Server& srv);

    void    onConnect(MPlexServer::Client client) override;
    void    onDisconnect(MPlexServer::Client client) override;
    void    onMessage(MPlexServer::Message msg) override;
    
    void    process_password(std::string, MPlexServer::Client);

    void    add_user(User);

private:
    MPlexServer::Server&    srv_instance_;
    std::vector<User>       all_server_users_;
    
};

class User
{
public:
    User() = delete;
    User(MPlexServer::Client);
    ~User();

private:
    std::string           nickname_;    
    std::string           username_;
    MPlexServer::Client&  client_;
    bool                  authenticated_;
    std::vector<Channel>  channels_;
};

class Channel
{
public:
    Channel();
    ~Channel();
    
private:
    std::string         chan_name_;
    std::vector<User>   all_channel_users_;
    std::vector<User>   operators_;
};

enum    msgType {
    PASS,
    CAP,
    NICK,
    USER,
    JOIN,
    PING,
};