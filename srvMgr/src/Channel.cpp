#include <utility>

#include "../include/SrvMgr.h"

Channel::Channel(std::string  chan_name, std::string chan_creator) : chan_name_(std::move(chan_name)) {
    chan_nicks_.insert(chan_creator);
    chan_ops_.insert(chan_creator);
    // topic_ = "";
}

std::string Channel::get_channel_name() {
    return chan_name_;
}
std::string Channel::get_channel_topic() {
    return topic_;
}
std::string Channel::get_user_nicks_str() {
    std::string all_nicks;
    for (const auto& op : chan_ops_) {
        all_nicks += "@" + op + " ";
    }
    for (const auto& nick : chan_nicks_) {
        if (chan_ops_.find(nick) == chan_ops_.end()) {
            all_nicks += nick + " ";
        }
    }
    return all_nicks;
}

std::unordered_set<std::string> Channel::get_chan_nicks() const {
    return chan_nicks_;
}


void    Channel::add_operator(std::string op) {
    chan_ops_.emplace(op);
}

int    Channel::remove_operator(std::string op) {
    return chan_ops_.erase(op);
}

void    Channel::add_nick(std::string nick) {
    chan_nicks_.insert(nick);
}

int    Channel::remove_nick(std::string nick) {
    return chan_nicks_.erase(nick);
}