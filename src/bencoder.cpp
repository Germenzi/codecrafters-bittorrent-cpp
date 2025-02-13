#include <sstream>

#include "bencoder.hpp"

using json = nlohmann::json;

namespace {

std::string bencode_detected(const json &what);


std::string bencode_integer(const json &what) {
    assert(what.is_number_integer());

    std::stringstream result_stream {};
    result_stream << 'i' << what.get<int64_t>() << 'e';
    return result_stream.str();
}


std::string bencode_list(const json &what) {
    assert(what.is_array());

    std::stringstream result_stream {};
    result_stream << 'l';
    for (const json &i : what)
        result_stream << bencode_detected(i);
    result_stream << 'e';
    return result_stream.str();
}


std::string bencode_string(const json &what) {
    assert(what.is_string());

    std::stringstream result_stream {};
    const json::string_t *str = what.get_ptr<const json::string_t*>();
    result_stream << str->size() << ':' << *str;
    return result_stream.str();
}


std::string bencode_dictionary(const json &what) {
    assert(what.is_object());

    std::stringstream result_stream {};
    result_stream << 'd';
    for (const auto &i : what.items())
        result_stream << bencode_string(i.key()) << bencode_detected(i.value());
    result_stream << 'e';
    return result_stream.str();
}


std::string bencode_detected(const json &what) {
    switch (what.type()) {
    case json::value_t::string:
        return bencode_string(what);
    
    case json::value_t::number_integer:
        return bencode_integer(what);
    
    case json::value_t::array:
        return bencode_list(what);
    
    case json::value_t::object:
        return bencode_dictionary(what);
    }

    throw std::runtime_error("can't detect bencode type: " + what.dump());
}


}


std::string bit_torrent::bencode_json(const json &what) {
    return bencode_detected(what);
}

