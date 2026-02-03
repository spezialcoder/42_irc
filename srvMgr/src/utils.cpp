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
    // size_t  idx = s.find_first_of(' ');
    // std::cout << s.substr(0, idx) << std::endl;
    if (s == "PASS") {
        return PASS;
    }
    else {
        return -1;
    }
}