#ifndef SUN_IOSTREAMBUF_H
#define SUN_IOSTREAMBUF_H

#include <unistd.h>

#include <array>
#include <streambuf>
#include <type_traits>

namespace streamx {

template <std::size_t IBufsize=1024, 
std::size_t OBufsize=1024, 
typename = std::enable_if_t<static_cast<bool>(IBufsize) && static_cast<bool>(OBufsize)>> 
class sun_iostreambuf : public std::streambuf {
    int fd_;
    std::array<traits_type::char_type, IBufsize> input_buffer_;
    std::array<traits_type::char_type, OBufsize> output_buffer_;


    void dump_output_buffer();

protected:
    int underflow() override;
    int overflow(traits_type::int_type ch=traits_type::eof()) override;
    int sync() override;

public:
    sun_iostreambuf(int fd);
    virtual ~sun_iostreambuf() override;
};


template <std::size_t IBufsize, std::size_t OBufsize, typename T>
sun_iostreambuf<IBufsize, OBufsize, T>::sun_iostreambuf(int fd) : fd_(fd) {
    setg(input_buffer_.data(), 
        input_buffer_.data(), 
        input_buffer_.data());
    
    setp(output_buffer_.data(), output_buffer_.data()+output_buffer_.size());
}


template <std::size_t IBufsize, std::size_t OBufsize, typename T>
sun_iostreambuf<IBufsize, OBufsize, T>::~sun_iostreambuf() {
    dump_output_buffer();
    close(fd_);
}


template <std::size_t IBufsize, std::size_t OBufsize, typename T>
void sun_iostreambuf<IBufsize, OBufsize, T>::dump_output_buffer() {
    int bytes_to_write = pptr() - pbase();
    if (bytes_to_write == 0) return;
    int wrote;
    if ((wrote = write(fd_, output_buffer_.data(), bytes_to_write)) == -1)
        return;
    
    int bytes_remains = bytes_to_write - wrote;

    if (bytes_remains == 0) {
        setp(output_buffer_.data(), output_buffer_.data()+output_buffer_.size());
    } else {
        setp(pbase()+wrote, epptr());
        pbump(bytes_remains);
    }
}


template <std::size_t IBufsize, std::size_t OBufsize, typename T>
int sun_iostreambuf<IBufsize, OBufsize, T>::underflow() {
    if (gptr() == egptr()) { 
        int bytes_read;
        if ((bytes_read = read(fd_, input_buffer_.data(), input_buffer_.size())) == -1) 
            return traits_type::eof();
        setg(input_buffer_.data(), input_buffer_.data(), input_buffer_.data()+bytes_read);
    }
    
    // no else
    if (gptr() != egptr())
        return traits_type::to_int_type(*gptr());
    
    return traits_type::eof();
}


template <std::size_t IBufsize, std::size_t OBufsize, typename T>
int sun_iostreambuf<IBufsize, OBufsize, T>::overflow(traits_type::int_type ch) {
    if (pptr() == epptr())
        dump_output_buffer();
    
    // no else
    if (pptr() != epptr()) {
        if (ch != traits_type::eof()) {
            *pptr() = traits_type::to_char_type(ch);
            pbump(1);
        }
        // if pptr() equals epptr(), eof() or not?
        return traits_type::not_eof(ch);
    }
    
    return traits_type::eof();
}


template <std::size_t IBufsize, std::size_t OBufsize, typename T>
int sun_iostreambuf<IBufsize, OBufsize, T>::sync() {
    dump_output_buffer();
    return gptr() == egptr();
}

}
#endif