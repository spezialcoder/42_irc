#include <utility>

#include "../include/SrvMgr.h"

Channel::Channel(std::string  chan_name) : chan_name_(std::move(chan_name)) {
}

