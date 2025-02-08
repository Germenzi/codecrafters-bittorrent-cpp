#ifndef BENCODE_PARSER_HPP
#define BENCODE_PARSER_HPP

#include <string>
#include "lib/nlohmann/json.hpp"

namespace bit_torrent {

class bencode_parser {
    std::string_view remains_;

    enum class bencode_types : int {
        TYPE_NONE = -1,
        TYPE_STRING,
        TYPE_INT,
        TYPE_LIST,
        TYPE_DICT
    };

    nlohmann::json parse_advance_integer();
    nlohmann::json parse_advance_string();
    nlohmann::json parse_advance_list();
    nlohmann::json parse_advance_dictionary();
    nlohmann::json parse_advance_detected();

    bencode_types detect_current_type() const;

public:
    nlohmann::json parse(const std::string &encoded);
};

}

#endif