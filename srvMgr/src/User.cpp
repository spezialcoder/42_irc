#include "../../server/include/mplexserver.h"
#include "../include/SrvMgr.h"

User::User(MPlexServer::Client client) : client_(client), authenticated_(false)
{
}
User::User() : authenticated_(false)
{
}
User::~User()
{
}


void        User::set_authentication(bool authentification_status) {
    authenticated_ = authentification_status;
}
bool        User::is_authenticated() {
    return authenticated_;
}

void        User::set_nickname(std::string nickname) {
    nickname_ = nickname;
}
std::string User::get_nickname() {
    return nickname_;
}
void        User::set_username(std::string username) {
    username_ = username;
}
std::string User::get_username() {
    return username_;
}
