#pragma once

#include <string>
#include <vector>

std::vector<std::string>    process_message(std::string);
void                        strip_trailing_rn(std::string& s);
int                         get_msg_type(std::string& s);