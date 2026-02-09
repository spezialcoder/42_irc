//
// Created by daspring on 2/9/26.
//

#pragma once

#include "User.h"

class Channel
{
public:
    Channel() = default;
    Channel(std::string  chan_name);
    Channel(const Channel& other) = default;
    ~Channel() = default;

    Channel&    operator=(const Channel& other) = default;

private:
    std::string                 chan_name_;
    std::map<std::string, User> all_channel_users_;
    // std::vector<User>           operators_;
};