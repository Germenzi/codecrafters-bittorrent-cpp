#include "bencode_parser.hpp"

using json = nlohmann::json;

json bit_torrent::bencode_parser::parse(const std::string &source) {
    remains_ = std::string_view {source};
    return parse_advance_detected();
}


json bit_torrent::bencode_parser::parse_advance_string() {
    assert(std::isdigit(remains_.front()));

    std::size_t colon_idx = remains_.find(':');
    if (colon_idx == std::string_view::npos)
        throw std::runtime_error {"parse_string: invalid string format at: " + std::string{remains_}};
    
    int64_t charcount = std::stoll(std::string{remains_.substr(0, colon_idx)});
    std::string result_string = std::string {remains_.substr(colon_idx+1, charcount)};

    remains_.remove_prefix(colon_idx+1+charcount);

    return json(result_string);
}


json bit_torrent::bencode_parser::parse_advance_integer() {
    assert(remains_.front() == 'i');

    std::size_t end_idx = remains_.find('e');
    if (end_idx == std::string_view::npos)
        throw std::runtime_error {"parse_integer: ending symbol 'e' not found at: " + std::string{remains_}};
    
    int64_t number = std::stoll(std::string{remains_.substr(1, end_idx-1)}); // from 1 because of 'i'

    remains_.remove_prefix(end_idx+1);
    return json(number);
}


json bit_torrent::bencode_parser::parse_advance_list() {
    assert(remains_.front() == 'l');

    remains_.remove_prefix(1); // removing 'l'
    json result = json::array();
    while (!remains_.empty() && remains_.front() != 'e') 
        result.push_back(parse_advance_detected());
    
    if (remains_.empty())
        throw std::runtime_error {"parse_list: list ending 'e' wasn't found"};
    
    remains_.remove_prefix(1); // removing 'e'
    return result;
}   


json bit_torrent::bencode_parser::parse_advance_dictionary() {
    assert(remains_.front() == 'd');

    remains_.remove_prefix(1); // removing 'd'
    json result = json::object();
    while (!remains_.empty() && remains_.front() != 'e') {
        if (detect_current_type() != bencode_types::TYPE_STRING)
            throw std::runtime_error {"parse_dictionary: string as a key expected: " + std::string{remains_}};
        
        json key = parse_advance_string();
        json value = parse_advance_detected();

        result[key] = value;
    }

    if (remains_.empty())
        throw std::runtime_error {"parse_dictionary: dictionary ending 'e' wasn't found"};
    
    remains_.remove_prefix(1); // removing 'e'
    return result;
}


json bit_torrent::bencode_parser::parse_advance_detected() {
    switch (detect_current_type()) {
    case bencode_types::TYPE_STRING:
        return parse_advance_string();
    
    case bencode_types::TYPE_INT:
        return parse_advance_integer();

    case bencode_types::TYPE_LIST:
        return parse_advance_list();

    case bencode_types::TYPE_DICT:
        return parse_advance_dictionary();
    };

    throw std::runtime_error {"parse_detected: invalid string format: " + std::string{remains_}};
}


bit_torrent::bencode_parser::bencode_types bit_torrent::bencode_parser::detect_current_type() const {
    if (remains_.empty())
        return bit_torrent::bencode_parser::bencode_types::TYPE_NONE;

    if (std::isdigit(remains_.front()))
        return bit_torrent::bencode_parser::bencode_types::TYPE_STRING;
    
    if (remains_.front() == 'i')
        return bit_torrent::bencode_parser::bencode_types::TYPE_INT;
    
    if (remains_.front() == 'l')
        return bit_torrent::bencode_parser::bencode_types::TYPE_LIST;
    
    if (remains_.front() == 'd')
        return bit_torrent::bencode_parser::bencode_types::TYPE_DICT;
    
    return bit_torrent::bencode_parser::bencode_types::TYPE_NONE;
}