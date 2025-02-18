#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "lib/nlohmann/json.hpp"
#include "bencode_parser.hpp"
#include "bencoder.hpp"
#include "sha1.hpp"
#include "tracker_request.hpp"


using json = nlohmann::json;

std::string readfile(const std::string &filename) {
    std::ifstream fin {filename};
    if (!fin.is_open())
        throw std::runtime_error("readfile: unable to open file " + filename);

    std::stringstream file_content;
    file_content << fin.rdbuf();
    fin.close();

    return file_content.str();
}


json decode_bencoded_value(const std::string &encoded_value) {
    bit_torrent::bencode_parser parser {};
    return parser.parse(encoded_value);
}


std::vector<std::string> extract_piece_hashes(const std::string &hash) {
    std::vector<std::string> result (hash.size() / (SHA1::DIGEST_SIZE*2));
    for (std::size_t offset = 0; offset < hash.size(); offset += SHA1::DIGEST_SIZE*2) {
        std::ostringstream hash_string;
        for (char ch : hash.substr(offset, SHA1::DIGEST_SIZE*2)) {
            hash_string << std::hex << std::setfill('0') << std::setw(2);
            hash_string << (+ch & 0xFF);
        }
        result.push_back(hash_string.str());
    }

    return result;
}


int main(int argc, char* argv[]) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    if (command == "decode") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }
        // You can use print statements as follows for debugging, they'll be visible when running tests.
        std::cerr << "Logs from your program will appear here!" << std::endl;

        // Uncomment this block to pass the first stage
        std::string encoded_value = argv[2];
        json decoded_value = decode_bencoded_value(encoded_value);
        std::cout << decoded_value.dump() << std::endl;
    } else if (command == "info") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " info <file>" << std::endl;
            return 1;
        }

        json torrent_info = decode_bencoded_value(readfile(argv[2]));

        SHA1 hasher {};
        hasher.update(bit_torrent::bencode_json(torrent_info["info"]));

        std::cout << "Tracker URL: " << torrent_info["announce"].get<std::string>() << '\n';
        std::cout << "Length: " << torrent_info["info"]["length"] << '\n';
        std::cout << "Info Hash: " << hasher.final() << '\n';
        std::cout << "Piece Length: " << torrent_info["info"]["piece length"] << '\n';
        std::cout << "Piece Hashes:\n";
        for (const std::string &i : extract_piece_hashes(torrent_info["info"]["pieces"]))
            std::cout << i << '\n';
    } else if (command == "peers") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " peers <file>" << std::endl;
            return 1;
        }
        
        json torrent_info = decode_bencoded_value(readfile(argv[2]));

        SHA1 hasher {};
        hasher.update(bit_torrent::bencode_json(torrent_info["info"]));

        std::string tracker_bencoded_info = bit_torrent::tracker_request::request(
            torrent_info["announce"].get<std::string>(), hasher.final(), std::string (20, '0'), 
            0, 0, torrent_info["info"]["length"].get<std::uint64_t>(), true);
        
        json tracker_info = decode_bencoded_value(tracker_bencoded_info);

        std::string peers = tracker_info["peers"].get<std::string>();
        for (int i = 0; i < peers.size()/6; ++i) {
            std::string address = peers.substr(i*6, 6);
            if (address.size() != 6)
                throw std::runtime_error("error reading peers, peer address size is " + std::to_string(address.size()) + " != 6");

            // use inet_ntop ??
            std::ostringstream ip_stream;
            // network order is BE, address[0] is the highest byte, 
            // like in IPv4 presentation format first number is the highest byte
            ip_stream << (static_cast<unsigned int>(address[0]) & 0xFF) << '.';
            ip_stream << (static_cast<unsigned int>(address[1]) & 0xFF) << '.';
            ip_stream << (static_cast<unsigned int>(address[2]) & 0xFF) << '.';
            ip_stream << (static_cast<unsigned int>(address[3]) & 0xFF);
            ip_stream << ':' << ntohs(*reinterpret_cast<const std::uint16_t*>(address.c_str()+4));
            std::cout << ip_stream.str() << '\n';
        }
    }
    
    else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
