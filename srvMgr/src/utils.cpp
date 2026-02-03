#include <string>

#include "../include/utils.h"

void    strip_trailing_rn(std::string& s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) {
        s.pop_back();
    }
}