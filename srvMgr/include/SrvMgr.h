#pragma once

#include <map>
#include <string>
#include <unordered_set>
#include <unordered_map>

#include "Channel.h"
#include "User.h"
#include "../../server/include/mplexserver.h"
// #include "./mplexserver.h"  // why does this worK??


/**
 * @brief Class to manage channels, users and their capabilities
*/
class SrvMgr : public MPlexServer::EventHandler {
public:
    SrvMgr() = delete;
    SrvMgr(MPlexServer::Server&, const std::string& server_password, const std::string& server_name);
    ~SrvMgr() = default;

    void    onConnect(MPlexServer::Client client) override;
    void    onDisconnect(MPlexServer::Client client) override;
    void    onMessage(MPlexServer::Message msg) override;

    void    try_to_log_in(User& user, const MPlexServer::Client& client) const;
    
    void    process_password(const std::string&, const MPlexServer::Client&, User&) const;
    void    process_cap(const std::string&, const MPlexServer::Client&, User&) const;
    void    process_nick(const std::string&, const MPlexServer::Client&, User&);
    void    process_user(std::string, const MPlexServer::Client&, User&) const;
    void    join_channel(std::string, const MPlexServer::Client&, User&);
    void    pong(const std::string &, const MPlexServer::Client &, const User&);


private:
    MPlexServer::Server&                        srv_instance_;
    const std::string                           server_password_;
    const std::string                           server_name_;
    std::map<int, User>                         server_users_;
    std::unordered_set<std::string>             server_nicks_;
    std::unordered_map<std::string, Channel>    server_channels_;
};


enum    cmdType {
    PASS,
    CAP,
    NICK,
    USER,
    JOIN,
    PART,
    PRIVMSG,
    NOTICE,
    MODE,
    INVITE,
    KICK,
    QUIT,
    PING,
    NO_TYPE_FOUND
};