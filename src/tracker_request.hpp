#ifndef TRACKER_REQUEST_H
#define TRACKER_REQUEST_H

#include <string>


namespace bit_torrent {

class tracker_request {
public:
    static std::string request(const std::string &url, 
        const std::string &info_hash, const std::string &peer_id, std::size_t uploaded, 
        std::size_t downloaded, std::uint64_t left, bool compact);

};

}

#endif