#include "../../server/include/mplexserver.h"
#include "../include/SrvMgr.h"

User::User(MPlexServer::Client& client) : client_(client)
{
}

[[nodiscard]] bool User::is_logged_in() const {
    return is_logged_in_;
}

void User::set_as_logged_in(bool is_logged_in) {
    is_logged_in_ = is_logged_in;
}

[[nodiscard]] bool User::password_provided() const {
    return password_provided_;
}

void User::set_password_provided(bool password_provided) {
    password_provided_ = password_provided;
}

[[nodiscard]] bool User::cap_negotiation_ended() const {
    return cap_negotiation_ended_;
}

void User::set_cap_negotiation_ended(bool cap_negotiation_ended) {
    cap_negotiation_ended_ = cap_negotiation_ended;
}

MPlexServer::Client User::get_client() const {
    return client_;
}

void        User::set_nickname(std::string nickname) {
    nickname_ = nickname;
}
std::string User::get_nickname() const {
    return nickname_;
}
void        User::set_username(std::string username) {
    username_ = username;
}
std::string User::get_username() {
    return username_;
}
void        User::set_hostname(std::string hostname) {
    hostname_ = hostname;
}
std::string User::get_hostname() {
    return hostname_;
}

std::string User::get_signature() const {
    // std::cout << nickname_ + "!" + username_ + "@" + hostname_ << std::endl;
    return nickname_ + "!" + username_ + "@" + hostname_;
}