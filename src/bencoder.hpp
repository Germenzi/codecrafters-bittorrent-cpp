#ifndef BENCODER_H
#define BENCODER_H

#include <string>

#include "lib/nlohmann/json.hpp"

namespace bit_torrent {

std::string bencode_json(const nlohmann::json& what);


}

#endif