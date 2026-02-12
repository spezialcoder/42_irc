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


// to do :  -generic error code ERR_UNKNOWNERROR if we don't handle the given params
//          -almost all commands should check if user is registered - else return ERR_NOTREGISTERED
//
//          -nick:  change nick in channel lists, too
//                  update names for other channel users
//
//          -join:  what if password is given or required?
//                  what if multiple channels should be created?
//                  chan names should only start with & or #
//          -kick
//          -invite
//          -topic
//          -mode
//
// done:    -ping
//          -quit
//          -part
//

SrvMgr::SrvMgr(MPlexServer::Server& srv, const string& server_password, const string& server_name) : srv_instance_(srv), server_password_(server_password), server_name_(server_name) {
}

void    SrvMgr::onConnect(MPlexServer::Client client) {
    cout << "[CONNECT] New client: " << client.getIpv4() << ":" << client.getPort() << endl;
    server_users_.emplace(client.getFd(), User(client));
}

void    SrvMgr::onDisconnect(MPlexServer::Client client) {
    User&       user = server_users_[client.getFd()];
    std::string nick = user.get_nickname();

    cout << "[DISCONNECT] " << nick << " (" << client.getIpv4() << ":" << client.getPort() << ") left" << endl;



    server_users_.erase(client.getFd());
    server_nicks_.erase(nick);

    // Broadcast QUIT message in IRC format
    if (user.get_farewell_message().empty()) {
        srv_instance_.broadcast(":" + nick + " QUIT :Client disconnected\r\n");
    } else {
        srv_instance_.broadcast(":" + nick + " " + user.get_farewell_message() + "\r\n");
    }
}

void    SrvMgr::onMessage(const MPlexServer::Message msg) {
    const MPlexServer::Client&  client = msg.getClient();
    User&                       user = server_users_[client.getFd()];
    std::vector<string>         msg_parts = process_message(msg.getMessage());
    const int                   command = get_msg_type(msg_parts[0]);


    for (auto s : msg_parts) {
        cout << "message parts '" << s << "'" << endl;
    }
    cout << "command: " << command << endl;

    switch (command) {
        case cmdType::PASS:
            process_password(msg_parts[1], client, user);
            break;
        case cmdType::CAP:
            process_cap(msg_parts[1], client, user);
            break;
        case cmdType::NICK:
            process_nick(msg_parts[1], client, user);
            break;
        case cmdType::USER:
            process_user(msg_parts[1], client, user);
            break;
        case cmdType::JOIN:
			process_join(msg_parts[1], client, user);
            break;
        case cmdType::PART:
            process_part(msg_parts[1], client, user);
            break;
        case cmdType::PRIVMSG:
            process_privmsg(msg_parts[1], client, user);
            break;
        case cmdType::TOPIC:
            process_topic(msg_parts[1], client, user);
            break;
        case cmdType::MODE:
            break;
        case cmdType::INVITE:
            break;
        case cmdType::KICK:
            break;
        case cmdType::QUIT:
			process_quit(msg_parts[1], client, user);
            break;
        case cmdType::PING:
            pong(msg_parts[1], client, user);
            break;
        default:
            cout << "no cmd_type found.\n";
    }

}

void    SrvMgr::try_to_log_in(User &user, const MPlexServer::Client &client) const {
    if (user.get_nickname().empty() || user.get_username().empty() ||
        !user.password_provided() || !user.cap_negotiation_ended()) {
        return ;
    }
    user.set_as_logged_in(true);
    const string nick = user.get_nickname();
    srv_instance_.sendTo(client, ":" + server_name_ + " " + RPL_WELCOME + " " + nick + " :Welcome to our single-server IRC network, " + user.get_signature() + "\r\n");
    srv_instance_.sendTo(client, ":" + server_name_ + " " + RPL_YOURHOST + " " + nick + " :Your host is " + server_name_ + ", running version 1.0.\r\n");
    srv_instance_.sendTo(client, ":" + server_name_ + " " + RPL_CREATED + " " + nick + " :This server was created today.\r\n");
    srv_instance_.sendTo(client, ":" + server_name_ + " " + RPL_MYINFO + " " + nick + " :server 1.0 o o\r\n");
}

void    SrvMgr::process_password(const std::string& provided_password, const MPlexServer::Client& client, User& user) const {
    if (user.is_logged_in()) {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_ALREADYREGISTERED + " " + user.get_nickname() + " " + ":You may not reregister\r\n");
        return ;
    }
    if (provided_password == server_password_) {
        user.set_password_provided(true);
    }
    else {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_PASSWDMISMATCH + " * " + ":Password incorrect\r\n");
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_NOTREGISTERED + " * " + ":You have not registered\r\n");
        srv_instance_.sendTo(client, "ERROR :Closing Link: " + client.getIpv4() + " (Password incorrect)\r\n");
        srv_instance_.disconnectClient(client);
        return ;
    }
    if (!user.is_logged_in()) {
        try_to_log_in(user ,client);
    }
}

void    SrvMgr::process_cap(const string& s, const MPlexServer::Client& client, User& user) const {
    if (s == "END") {
        user.set_cap_negotiation_ended(true);
    }
    else {
        srv_instance_.sendTo(client, "CAP * LS :\r\n");
    }
    if (!user.is_logged_in()) {
        try_to_log_in(user ,client);
    }
}

void    SrvMgr::process_nick(const string& s, const MPlexServer::Client& client, User& user) {
    string	new_nick;
	string	old_nick = user.get_nickname();
	string	old_signature = user.get_signature();

	if (!user.is_logged_in()) {
		old_nick = "*";
}
	if (s.find_first_of("#:; ") == s.npos) {
		new_nick = s;
	} else {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_ERRONEUSNICKNAME + " " + old_nick + " " + s + ":Erroneous nickname, it may not contain \"#:; \"\r\n");
        return ;
    }
    if (server_nicks_.find(new_nick) == server_nicks_.end()) {
        if (!(user.get_nickname().empty())) {
            server_nicks_.erase(user.get_nickname());
        }
        server_nicks_.emplace(new_nick, client.getFd());
        user.set_nickname(new_nick);
		if (user.is_logged_in()) {
        	srv_instance_.sendTo(client, ":" + old_signature + " NICK :" + new_nick + "\r\n");
		}
    }
    else {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_NICKNAMEINUSE + " " + old_nick + " " + s + ":Nickname is already in use\r\n");
    }
    if (!user.is_logged_in()) {
        try_to_log_in(user ,client);
    }
}

void    SrvMgr::process_user(string s, const MPlexServer::Client& client, User& user) const {
    if (user.is_logged_in()) {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_ALREADYREGISTERED + " " + user.get_nickname() + " " + ":You may not reregister\r\n");
        return ;
    }

    string username = split_off_before_del(s, ' ');
    string hostname = split_off_before_del(s, ' ');

    if (username.empty() || hostname.empty()) {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_NEEDMOREPARAMS + " * " + ":Not enough parameters for user registration\r\n");
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_NOTREGISTERED + " * " + ":You have not registered\r\n");
        return ;
    }

    user.set_username(username);
    user.set_hostname(hostname);

    cout << "process_user: username: " << username
        << ", hostname: " << hostname << endl;
    if (!user.is_logged_in()) {
        try_to_log_in(user ,client);
    }

    // if no password was provided after CAP, NICK and USER registration fails and we terminate
    if (!user.password_provided()) {
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_PASSWDMISMATCH + " * " + ":Password incorrect\r\n");
        srv_instance_.sendTo(client, ":" + server_name_ + " " + ERR_NOTREGISTERED + " * " + ":You have not registered\r\n");
        srv_instance_.sendTo(client, "ERROR :Closing Link: " + client.getIpv4() + " (Password incorrect)\r\n");
        srv_instance_.disconnectClient(client);
    }
}

void    SrvMgr::process_join(string s, const MPlexServer::Client& client, User& user) {
	string chan_name = s;

    if (server_channels_.find(chan_name) == server_channels_.end()) {
        server_channels_.emplace(chan_name, Channel(chan_name, user.get_nickname()));
    }
    Channel& channel = server_channels_[chan_name];
    channel.add_nick(user.get_nickname());
    send_channel_command_ack(channel, client, user);
    send_channel_greetings(channel, client, user);
}

void    SrvMgr::process_part(string s, const MPlexServer::Client& client, User& user) {
    (void)  client;
    string  nick = user.get_nickname();

    string chan_name = split_off_before_del(s, ' ');
    string reason = split_off_before_del(s, ' ');

    if (server_channels_.find(chan_name) == server_channels_.end()) {
        string msg = ":" + server_name_ + " " + ERR_NOSUCHCHANNEL + " " + user.get_nickname() + " " + chan_name + " :No such channel";
        send_to_one(nick, msg);
        return ;
    }

    Channel& channel = server_channels_[chan_name];
    if (channel.get_chan_nicks().find(user.get_nickname()) == channel.get_chan_nicks().end()) {
        string msg = ":" + server_name_ + " " + ERR_NOTONCHANNEL + " " + user.get_nickname() + " " + chan_name + " :You're not on that channel";
        send_to_one(nick, msg);
        return ;
    }

    string  message = ":" + user.get_signature() + " PART " + chan_name + " " + reason;
    send_to_chan_all(channel, message);
    remove_user_from_channel(channel, nick);
}

void    SrvMgr::process_privmsg(std::string s, const MPlexServer::Client& client, User& user) {
    (void)  client;
    string  target = split_off_before_del(s, ' ');
    string  message = s;
    string  nick = user.get_nickname();

    if (message.empty()) {
        string err_msg = ":" + server_name_ + " " + ERR_NOTEXTTOSEND + " " + nick + " :No text to send";
        send_to_one(user, err_msg);
        return ;
    }

    if (target[0] != '#' && target[0] != '&') {
        if (server_nicks_.find(target) == server_nicks_.end()) {
            string err_msg = ":" + server_name_ + " " + ERR_NOSUCHNICK + " " + nick + " " + target + " :No such nick";
            send_to_one(user, err_msg);
            return ;
        } else {
            message = ":" + user.get_signature() + " PRIVMSG " + target + " " + message;
            send_to_one(target, message);
        }
    } else {
        auto    chan_it = server_channels_.find(target);
        if (chan_it == server_channels_.end()) {
            string err_msg = ":" + server_name_ + " " + ERR_NOSUCHCHANNEL + " " + nick + " " + target + " :No such channel";
            send_to_one(user, err_msg);
            return ;
        } else {
            Channel& channel = chan_it->second;
            if (channel.get_chan_nicks().find(nick) == channel.get_chan_nicks().end()) {
                string err_msg = ":" + server_name_ + " " + ERR_NOTONCHANNEL + " " + nick + " " + target + " :You're not on that channel";
                send_to_one(user, err_msg);
                return ;
            }
            message = ":" + user.get_signature() + " PRIVMSG " + target + " " + message;
            send_to_chan_all_but_one(channel, message, nick);
        }
    }
}

void SrvMgr::process_topic(std::string s, const MPlexServer::Client& client, User& user) {
    (void)  client;
    (void)  user;
    string  chan_name = split_off_before_del(s, ' ');
    string  new_topic = split_off_before_del(s, ' ');
    if (new_topic.empty()) {
        string  topic = ":" + server_name_ + " " + RPL_TOPIC + " " + user.get_nickname() + " " + chan_name + " " + server_channels_[chan_name].get_channel_topic();
        send_to_one(user.get_nickname(), topic);
    } else {
        server_channels_[chan_name].set_channel_topic(s);
    }
}

void    SrvMgr::process_quit(string s, const MPlexServer::Client &client, User& user) {
	user.set_farewell_message(s);
	srv_instance_.disconnectClient(client);
}

void    SrvMgr::pong(const string &s, const MPlexServer::Client &client, const User&) {
    string nick = server_users_[client.getFd()].get_nickname();
    srv_instance_.sendTo(client, ":" + server_name_ + " PONG " + server_name_ + " " + s + "\r\n");
    cout << ":" + server_name_ + " PONG " + server_name_ + " :" + s << endl;
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
void    SrvMgr::send_to_chan_all(const Channel& channel, const std::string& msg) const {
    auto set_of_nicks = channel.get_chan_nicks();
    std::vector<MPlexServer::Client> clients = create_client_vector(set_of_nicks);
    srv_instance_.multisend(clients, msg + "\r\n");
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

void    SrvMgr::remove_op_from_channel(Channel& channel, std::string &op) {
    channel.remove_operator(op);
    remove_nick_from_channel(channel, op);
}
void    SrvMgr::remove_nick_from_channel(Channel& channel, std::string &nick) {
    channel.remove_nick(nick);
    if (channel.get_chan_nicks().empty()) {
        server_channels_.erase(channel.get_channel_name());
    }
}

void SrvMgr::remove_user_from_channel(Channel &channel, std::string &nick) {
    remove_nick_from_channel(channel, nick);
}
