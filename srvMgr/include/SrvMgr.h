#pragma once

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "./mplexserver.h"

class   Channel;
class   User;

/**
 * @brief Class to manage users and their capabilities
*/
class SrvMgr : public MPlexServer::EventHandler {
public:
    SrvMgr(MPlexServer::Server&, std::string);

    void    onConnect(MPlexServer::Client client) override;
    void    onDisconnect(MPlexServer::Client client) override;
    void    onMessage(MPlexServer::Message msg) override;
    
    void    process_password(std::string, MPlexServer::Client);
    void    process_cap(std::string, MPlexServer::Client);
    void    process_nick(std::string, MPlexServer::Client);
    void    process_user(std::string, MPlexServer::Client);


private:
    MPlexServer::Server&            srv_instance_;
    std::string                     server_password_; // Server password
    std::map<int, User>             server_users_;
    std::unordered_set<std::string> server_nicks;
};

class User
{
public:
    User();
    User(MPlexServer::Client);
    ~User();

    bool        is_authenticated();
    void        set_authentication(bool);
    void        set_nickname(std::string);
    std::string get_nickname();
    void        set_username(std::string);
    std::string get_username();

private:
    MPlexServer::Client   client_; 
    bool                  authenticated_;
    std::string           nickname_;    
    std::string           username_;
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

enum    MsgType {
    PASS,
    CAP,
    NICK,
    USER,
    JOIN,
    PING,
};