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
    void    pong(const std::string &, const MPlexServer::Client &, const User&);


private:
    MPlexServer::Server&            srv_instance_;
    const std::string               server_password_;
    const std::string               server_name_;
    std::map<int, User>             server_users_;
    std::unordered_set<std::string> server_nicks;
};

class User
{
public:
    User() = default;
    User(const User& other) = default;
    User(MPlexServer::Client&);
    ~User() = default;

    User& operator=(const User& other) = default;


    bool        is_authenticated();
    void        set_authentication(bool);
    void        set_nickname(std::string);
    std::string get_nickname() const;
    void        set_username(std::string);
    std::string get_username();
    void        set_hostname(std::string);
    std::string get_hostname();

    [[nodiscard]] bool  is_logged_in() const;
    void                set_as_logged_in(bool is_logged_in);
    [[nodiscard]] bool  password_provided() const;
    void                set_password_provided(bool password_provided);
    [[nodiscard]] bool  cap_negotiation_ended() const;
    void                set_cap_negotiation_ended(bool cap_negotiation_ended);

private:
    MPlexServer::Client     client_{};
    bool                    is_logged_in_ = false;
    bool                    password_provided_ = false;
    bool                    cap_negotiation_ended_ = false;
    bool                    authenticated_ = false;
    std::string             nickname_{"*"};
    std::string             username_{};
    std::string             hostname_{};
    std::vector<Channel>    channels_{};
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

enum    cmdType {
    PASS,
    CAP,
    NICK,
    USER,
    JOIN,
    PRIVMSG,
    NOTICE,
    MODE,
    INVITE,
    KICK,
    QUIT,
    PING,
    NO_TYPE_FOUND
};