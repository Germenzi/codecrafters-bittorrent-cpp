#ifndef BENCODE_PARSER_HPP
#define BENCODE_PARSER_HPP

#include <string>
#include "lib/nlohmann/json.hpp"

namespace bit_torrent {

class bencode_parser {
    std::string_view remains_;
    nlohmann::json parse_advance_integer();
    nlohmann::json parse_advance_string();
    nlohmann::json parse_advance_list();
    nlohmann::json parse_advance_dictionary();
    nlohmann::json parse_advance_detected();

public:
    nlohmann::json parse(const std::string &encoded);
};

}

#endif