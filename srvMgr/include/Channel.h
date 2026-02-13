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

    std::string                     get_channel_name();
    std::string                     get_channel_topic();
    void                            set_channel_topic(std::string&);
    std::string                     get_user_nicks_str();
    std::unordered_set<std::string> get_chan_nicks() const;
    void                            add_operator(std::string);
    int                             remove_operator(std::string);
    void                            add_nick(std::string);
    int                             remove_nick(std::string);
    bool                            has_chan_member(const std::string &nick);
    bool                            has_chan_op(const std::string &nick);

private:
    std::string                     chan_name_;
    std::string                     topic_{":"};
    std::unordered_set<std::string> chan_nicks_;
    std::unordered_set<std::string> chan_ops_;
};