#include <utility>

#include "../include/SrvMgr.h"

Channel::Channel(std::string  chan_name, std::string chan_creator) : chan_name_(std::move(chan_name)) {
    all_channel_users_.insert(chan_creator);
    operators_.insert(chan_creator);
    // topic_ = "";
}

std::string Channel::get_channel_name() {
    return chan_name_;
}
std::string Channel::get_channel_topic() {
    return topic_;
}
std::string Channel::get_all_user_nicks() {
    std::string all_nicks = "";
    for (auto nick : all_channel_users_) {
        all_nicks += nick;
        all_nicks += " ";
    }
    return all_nicks;
}

