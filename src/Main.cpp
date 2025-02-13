#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "lib/nlohmann/json.hpp"
#include "bencode_parser.hpp"
#include "bencoder.hpp"
#include "sha1.hpp"

using json = nlohmann::json;


json decode_bencoded_value(const std::string& encoded_value) {
    bit_torrent::bencode_parser parser {};
    return parser.parse(encoded_value);
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
            std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
            return 1;
        }

        std::ifstream fin {argv[2]};
        if (!fin.is_open()) {
            std::cerr << "unable to open " << argv[2] << std::endl;
            return 1;
        }

        std::stringstream file_content;
        file_content << fin.rdbuf();
        fin.close();

        json torrent_info = decode_bencoded_value(file_content.str());

        SHA1 hasher {};
        hasher.update(bit_torrent::bencode_json(torrent_info["info"]));

        std::cout << "Tracker URL: " << std::string (torrent_info["announce"]) << '\n';
        std::cout << "Length: " << torrent_info["info"]["length"] << '\n';
        std::cout << "Info Hash: " << hasher.final() << '\n';
    } else {
        std::cerr << "unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
