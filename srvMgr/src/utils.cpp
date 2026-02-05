#include <string>
#include <vector>

#include "../include/utils.h"
#include "../include/SrvMgr.h"

void    strip_trailing_rn(std::string& s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) {
        s.pop_back();
    }
}

std::vector<std::string>    process_message(std::string s) {
    size_t                      idx;
    std::vector<std::string>    msg_parts;

    strip_trailing_rn(s);
    idx = s.find_first_of(' ');
    msg_parts.push_back(s.substr(0, idx));
    msg_parts.push_back(s.substr(idx + 1, s.length() - idx));

    return msg_parts;
}

int     get_msg_type(std::string& s) {
    if (s == "PASS") {
        return cmdType::PASS;
    } else if (s == "CAP") {
        return cmdType::CAP;
    } else if (s == "NICK") {
        return cmdType::NICK;
    } else if (s == "USER") {
        return cmdType::USER;
    } else if (s == "JOIN") {
        return cmdType::JOIN;
    } else if (s == "PRIVMSG") {
        return cmdType::PRIVMSG;
    } else if (s == "NOTICE") {
        return cmdType::NOTICE;
    } else if (s == "MODE") {
        return cmdType::MODE;
    } else if (s == "INVITE") {
        return cmdType::INVITE;
    } else if (s == "KICK") {
        return cmdType::KICK;
    } else if (s == "PING") {
        return cmdType::PING;
    }
    else {
        return cmdType::NO_TYPE_FOUND;
    }
}