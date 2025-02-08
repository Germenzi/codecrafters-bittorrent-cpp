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

    json result = json::array();
    remains_.remove_prefix(1); // removing 'l'
    while (!remains_.empty() && remains_.front() != 'e') 
        result.push_back(parse_advance_detected());
    
    if (remains_.empty())
        throw std::runtime_error {"parse_list: list ending 'e' wasn't found"};
    
    remains_.remove_prefix(1); // removing 'e'
    return result;
}   


json bit_torrent::bencode_parser::parse_advance_detected() {
    std::string_view::value_type current = remains_.front();

    if (std::isdigit(current))
        return parse_advance_string();
    
    if (current == 'i')
        return parse_advance_integer();
    
    if (current == 'l')
        return parse_advance_list();

    throw std::runtime_error {"parse_detected: invalid string format: " + std::string{remains_}};
}
