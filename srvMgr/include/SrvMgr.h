#pragma once

#include <string>

#include "./mplexserver.h"

// class Channel;

/**
 * @brief Class to manage users and their capabilities
*/
class UsrMgnt : public MPlexServer::EventHandler {
public:
    UsrMgnt(MPlexServer::Server& srv);

    void    onConnect(MPlexServer::Client client) override;
    void    onDisconnect(MPlexServer::Client client) override;
    void    onMessage(MPlexServer::Message msg) override;

private:
    MPlexServer::Server& srv_instance;
    
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
    bool                  is_authenticated_;
    MPlexServer::Client&  client_;
    std::vector<Channel>  channels_;
};

class Channel
{
public:
    Channel();
    ~Channel();
    
private:
    std::string         chan_name_;
    std::vector<User>   all_users_;
    std::vector<User>   operators_;
};
