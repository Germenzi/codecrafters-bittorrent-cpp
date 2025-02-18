#include "sys/socket.h"
#include "string.h"
#include "errno.h"
#include "netdb.h"
#include <sstream>
#include <array>
#include <vector>
#include <string>
#include <map>

#include "sha1.hpp"
#include "tracker_request.hpp"
#include "sun_iostreambuf.hpp"

namespace {

struct http_response {
    std::string version;
    std::size_t status_code;
    std::string comment;
    std::map<std::string, std::string> headers; 
};


const char *crlf = "\r\n";


template <typename ArrT>
std::string url_encode(const ArrT &data) {
    std::stringstream url_encoded_stream {};

    for (char ch : data) {
        if (std::isdigit(ch) || std::isalpha(ch))
            url_encoded_stream << ch;
        else if (ch == ' ')
            url_encoded_stream << '+';
        else
            url_encoded_stream << '%' << std::hex << std::setfill('0') << std::setw(2) << (+ch & 0xFF);
    }

    return url_encoded_stream.str();
}


std::string read_until_crlf(std::istream &istream, std::size_t size_limit=8000) {
    std::string result;
    std::size_t chars_readed = 0;
    int matches = 0;
    std::string::value_type current;
    while (matches != 2) {
        if (!istream.get(current)) break;
        if (++chars_readed == size_limit)
            throw std::runtime_error("reading until crlf size limit exceeded");
        
        switch (matches) {
        case 0:
            if (current == '\r') {
                ++matches;
            } else {
                result.push_back(current);
            }
            break;
        
        case 1:
            if (current == '\n') {
                ++matches;
            } else {
                result.push_back(' '); // replacing CR with SP
                result.push_back(current);
                matches = 0;
            } 
            break;
        }
    }

    return result;
}



http_response get_http_response_from_stream(std::istream &istream) {
    http_response result;

    std::istringstream status_line_stream { read_until_crlf(istream) };
    status_line_stream >> result.version;
    status_line_stream >> result.status_code;
    result.comment = read_until_crlf(status_line_stream);

    std::string header;
    while (!(header = read_until_crlf(istream)).empty()) {
        std::string header_name, header_value;
        auto iter = header.begin(), iend = header.end();
        while (iter != iend && *iter != ':')
            header_name.push_back(*iter++);
        
        // skip colon
        if (iter != iend) ++iter; 
        // skip optional leading whitespace
        if (iter != iend && *iter == ' ') ++iter;
        
        
        while (iter != iend)
            header_value.push_back(*iter++);
        
        // trim optional trailing whitespace
        if (!header_value.empty() && header_value.back() == ' ') header_value.pop_back();
        result.headers.insert({header_name, header_value});
    }

    return result;
}



std::pair<std::string, std::string> get_hostport_from_httpurl(const std::string &url) {
    /* from RFC 1738, sec 5
    httpurl = "http://" hostport [ "/" hpath [ "?" search ]]
    hostport = host [ ":" port ]
    */

    if (!url.starts_with("http://"))
        throw std::runtime_error("get_address_from_httpurl: unable to extract node from httpurl: " + url);
    
    std::string_view url_view {url};
    url_view.remove_prefix(7); // remove "http://"

    std::string nodes, port;
    for (std::string *current = &nodes; !url_view.empty() && url_view.front() != '/'; url_view.remove_prefix(1)) {
        if (url_view.front() == ':') {
            current = &port;
            continue;
        }
        
        current->push_back(url_view.front());
    }

    return {nodes, port};
}


} // anon namespace


std::string bit_torrent::tracker_request::request(const std::string &url, 
        const std::string &info_hash, const std::string &peer_id, std::size_t uploaded, 
        std::size_t downloaded, std::uint64_t left, bool compact) {
        
    addrinfo hints, *addrlist;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;
    hints.ai_addr = NULL;
    //EAI_ADDRFAMILY;
    
    std::pair<std::string, std::string> hostport = 
        get_hostport_from_httpurl(url);

    if (int err = getaddrinfo(hostport.first.c_str(), hostport.second.c_str(), &hints, &addrlist); err != 0) 
        throw std::runtime_error("tracker_request: getaddrinfo error with code: " + std::string(gai_strerror(err)));


    int sock_fd;
    for (addrinfo *curr = addrlist; curr != NULL; curr = curr->ai_next) {
        sock_fd = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
        if (sock_fd == -1) continue;

        if (connect(sock_fd, curr->ai_addr, curr->ai_addrlen) != 0) {
            close(sock_fd);
            sock_fd = -1;
            continue;
        };

        break;
    }
    freeaddrinfo(addrlist);
    if (sock_fd == -1)
        throw std::runtime_error("tracker_request: unable to create active socket");
    
    std::string raw_digest (SHA1::DIGEST_SIZE, '\0');
    for (int i = 0; i < SHA1::DIGEST_SIZE; ++i)
        raw_digest[i] = std::stoi(info_hash.substr(2*i, 2), nullptr, 16);
    
    std::stringstream url_stream;
    url_stream << url<< '?'
        << "info_hash=" << url_encode(raw_digest) << '&'
        << "peer_id=" <<  peer_id << '&'
        << "port=" << 6881 << '&'
        << "uploaded=" << uploaded << '&'
        << "downloaded=" << downloaded << '&'
        << "left=" << left << '&'
        << "compact=" << static_cast<int>(compact)
        ;

    streamx::sun_iostreambuf socket_buffer {sock_fd};
    std::iostream socket_stream {&socket_buffer};
    socket_stream 
    // request line
    << "GET " << url_stream.str() << " HTTP/1.1" << crlf
    // headers
    << "Host: " << hostport.first << ':' << hostport.second << crlf
    << crlf
    // body
    << std::flush;

    http_response resp = get_http_response_from_stream(socket_stream);
    if (resp.status_code / 100 != 2) // not 2XX
        throw std::runtime_error("tracker_request: non successful response from server: " + 
            (std::ostringstream{} << resp.version << ' ' << resp.status_code << ' ' << resp.comment).str());
    
    if (resp.headers.find("Content-Length") == resp.headers.end())
        throw std::runtime_error("tracker_request: response doesn't have Content-Length header");
    
    std::size_t content_length = std::stoi(resp.headers.at("Content-Length"));
    std::string bencoded_response (content_length, '&');
    if (!socket_stream.read(bencoded_response.data(), content_length))
        throw std::runtime_error("tracker_request: unable to read enough bytes from response");

    if (socket_stream.gcount() != content_length)
        throw std::runtime_error("tracker_request: content_length is " + 
            std::to_string(content_length) + ", readed " +
            std::to_string(socket_stream.gcount()));

    return bencoded_response;
}   