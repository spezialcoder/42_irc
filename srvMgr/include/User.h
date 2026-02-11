//
// Created by daspring on 2/9/26.
//

#pragma once

#include <unordered_set>

#include "../../server/include/mplexserver.h"

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
    std::string get_signature() const;

    [[nodiscard]] bool  is_logged_in() const;
    void                set_as_logged_in(bool is_logged_in);
    [[nodiscard]] bool  password_provided() const;
    void                set_password_provided(bool password_provided);
    [[nodiscard]] bool  cap_negotiation_ended() const;
    void                set_cap_negotiation_ended(bool cap_negotiation_ended);

private:
    MPlexServer::Client             client_{};
    bool                            is_logged_in_ = false;
    bool                            password_provided_ = false;
    bool                            cap_negotiation_ended_ = false;
    std::string                     nickname_{};
    std::string                     username_{};
    std::string                     hostname_{};
    std::unordered_set<std::string> channel_names_{}; // really useful? even used?
};