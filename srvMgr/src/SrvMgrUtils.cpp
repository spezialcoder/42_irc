#include <vector>

#include "mplexserver.h"
#include "Channel.h"
#include "IRC_macros.h"
#include "SrvMgr.h"
#include "User.h"
#include "utils.h"

using std::cout;
using std::endl;
using std::string;

void    SrvMgr::try_to_log_in(User &user, const MPlexServer::Client &client) const {
    cout << "[LOGIN] Checking login: nick='" << user.get_nickname() << "', user='" << user.get_username() 
         << "', pass=" << user.password_provided() << ", cap_started=" << user.cap_negotiation_started()
         << ", cap_ended=" << user.cap_negotiation_ended() << endl;
    
    if (user.get_nickname().empty() || user.get_username().empty() ||
        !user.password_provided() || 
        (user.cap_negotiation_started() && !user.cap_negotiation_ended())) {
        cout << "[LOGIN] Requirements not met, login blocked" << endl;
        return ;
        }
    cout << "[LOGIN] ✓ All requirements met, logging in user '" << user.get_nickname() << "'" << endl;
    user.set_as_logged_in(true);
    const string nick = user.get_nickname();
    srv_instance_.sendTo(client, ":" + server_name_ + " " + RPL_WELCOME + " " + nick + " :Welcome to our single-server IRC network, " + user.get_signature() + "\r\n");
    srv_instance_.sendTo(client, ":" + server_name_ + " " + RPL_YOURHOST + " " + nick + " :Your host is " + server_name_ + ", running version 1.0.\r\n");
    srv_instance_.sendTo(client, ":" + server_name_ + " " + RPL_CREATED + " " + nick + " :This server was created today.\r\n");
    srv_instance_.sendTo(client, ":" + server_name_ + " " + RPL_MYINFO + " " + nick + " :server 1.0 o o\r\n");
}

void    SrvMgr::send_to_one(const User& user, const std::string& msg) {
    srv_instance_.sendTo(user.get_client(), msg + "\r\n");
}
void    SrvMgr::send_to_one(const string& nick, const std::string& msg) {
    auto    nick_it = server_nicks_.find(nick);
    if (nick_it == server_nicks_.end()) {
        return ;
    }
    auto    user_it = server_users_.find(nick_it->second);
    if (user_it == server_users_.end()) {
        return ;
    }
    send_to_one(user_it->second, msg);
}
void    SrvMgr::send_to_chan_all(const Channel& channel, const std::string& msg) const {
    auto set_of_nicks = channel.get_chan_nicks();
    std::vector<MPlexServer::Client> clients = create_client_vector(set_of_nicks);
    srv_instance_.multisend(clients, msg + "\r\n");
}
void    SrvMgr::send_to_chan_all_but_one(const Channel& channel, const std::string& msg, const std::string& origin_nick) const {
    auto set_of_nicks = channel.get_chan_nicks();
    set_of_nicks.erase(origin_nick);
    std::vector<MPlexServer::Client> clients = create_client_vector(set_of_nicks);
    srv_instance_.multisend(clients, msg + "\r\n");
}
void    SrvMgr::send_to_chan_all_but_one(const std::string& chan_name, const std::string& msg, const std::string& origin_nick) const {
    auto    chan_it = server_channels_.find(chan_name);
    if (chan_it == server_channels_.end()) {
        return ;
    }
    const Channel& channel = chan_it->second;
    send_to_chan_all_but_one(channel, msg, origin_nick);
}

void    SrvMgr::remove_op_from_channel(Channel& channel, std::string &op) {
    channel.remove_operator(op);
}
void    SrvMgr::remove_nick_from_channel(Channel& channel, std::string &nick) {
    channel.remove_nick(nick);
    if (channel.get_chan_nicks().empty()) {
        server_channels_.erase(channel.get_channel_name());
    }
}
void    SrvMgr::remove_user_from_channel(Channel &channel, std::string &nick) {
    remove_op_from_channel(channel, nick);
    remove_nick_from_channel(channel, nick);
}

void    SrvMgr::change_nick(const string &new_nick, const std::string& old_nick, User& user) {
    for (auto& channel_it : server_channels_) {
        Channel& channel = channel_it.second;
        if (channel.has_chan_member(old_nick)) {
            change_nick_in_channel(new_nick, old_nick, channel);
        }
    }
    if (!old_nick.empty()) {
        server_nicks_.erase(old_nick);
    }
    server_nicks_.emplace(new_nick, user.get_client().getFd());
    user.set_nickname(new_nick);
}
void    SrvMgr::change_nick_in_channel(const std::string &new_nick, const std::string &old_nick, Channel &channel) {
    string old_signature = server_users_[server_nicks_[old_nick]].get_signature();
    if (channel.has_chan_op(old_nick)) {
        channel.remove_operator(old_nick);
        channel.add_nick(new_nick);
    }
    if (channel.has_chan_member(old_nick)) {
        channel.remove_nick(old_nick);
        channel.add_nick(new_nick);
    }
    string msg = ":" + old_signature + " NICK :" + new_nick;
    cout << msg << endl;
    send_to_chan_all_but_one(channel, msg, new_nick);
}

void    SrvMgr::send_channel_command_ack(Channel& channel, const MPlexServer::Client& client, const User& user) {
    (void)  client;
    string  ack = ":" + user.get_nickname() + " JOIN :" + channel.get_channel_name();
    send_to_chan_all(channel, ack);
}
void    SrvMgr::send_channel_greetings(Channel& channel, const MPlexServer::Client& client, const User& user) {
    (void)  client;
    string  topic = ":" + server_name_ + " " + RPL_TOPIC + " " + user.get_nickname() + " " + channel.get_channel_name() + " " + channel.get_channel_topic();
    string  name_reply = ":" + server_name_ + " " + RPL_NAMREPLY + " " + user.get_nickname() + " = " + channel.get_channel_name() + " :" + channel.get_user_nicks_str();
    string  end_of_names = ":" + server_name_ + " " + RPL_ENDOFNAMES + " " + user.get_nickname() + " " + channel.get_channel_name() + " :End of /NAMES list.";

    send_to_one(user, topic);
    send_to_one(user, name_reply);
    send_to_one(user, end_of_names);
}

std::vector<MPlexServer::Client>    SrvMgr::create_client_vector(const std::unordered_set<std::string>& set_of_nicks) const {
    std::vector<MPlexServer::Client> clients;
    for (const string& nick : set_of_nicks) {
        auto nick_it = server_nicks_.find(nick);
        if (nick_it == server_nicks_.end()) {
            continue ;
        }

        int user_id = nick_it->second;

        auto user_it = server_users_.find(user_id);
        if (user_it == server_users_.end()) {
            continue ;
        }

        const User& user = user_it->second;
        MPlexServer::Client client = user.get_client();
        clients.push_back(client);
    }
    return clients;
}