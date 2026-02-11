//
// Created by daspring on 2/9/26.
//

#pragma once

#include "User.h"

class Channel
{
public:
    Channel() = default;
    Channel(std::string  chan_name, std::string chan_creator);
    Channel(const Channel& other) = default;
    ~Channel() = default;

    Channel&    operator=(const Channel& other) = default;

    std::string get_channel_name();
    std::string get_channel_topic();
    std::string get_all_user_nicks();

private:
    std::string                     chan_name_;
    std::string                     topic_{};
    std::unordered_set<std::string> all_channel_users_;
    std::unordered_set<std::string> operators_;
};