#ifndef BXZSTR_Z_STREAM_WRAPPER_HPP
#define BXZSTR_Z_STREAM_WRAPPER_HPP

#include <zlib.h>

#include <string>
#include <sstream>
#include <exception>

#include "stream_wrapper.hpp"

namespace bxz {
/// Exception class thrown by failed zlib operations.
class zException
    : public std::exception
{
public:
    zException(z_stream * zstrm_p, int ret)
        : _msg("zlib: ")
    {
        switch (ret)
        {
        case Z_STREAM_ERROR:
            _msg += "Z_STREAM_ERROR: ";
            break;
        case Z_DATA_ERROR:
            _msg += "Z_DATA_ERROR: ";
            break;
        case Z_MEM_ERROR:
            _msg += "Z_MEM_ERROR: ";
            break;
        case Z_VERSION_ERROR:
            _msg += "Z_VERSION_ERROR: ";
            break;
        case Z_BUF_ERROR:
            _msg += "Z_BUF_ERROR: ";
            break;
        default:
            std::ostringstream oss;
            oss << ret;
            _msg += "[" + oss.str() + "]: ";
            break;
        }
        _msg += zstrm_p->msg;
    }
    zException(const std::string msg) : _msg(msg) {}
    const char * what() const noexcept { return _msg.c_str(); }
private:
    std::string _msg;
}; // class zException

namespace detail {
    class z_stream_wrapper : public z_stream, public stream_wrapper {
    private:
	bool is_input;
	int ret;
    public:
	z_stream_wrapper(bool _is_input = true, int _level = Z_DEFAULT_COMPRESSION, int _flags = 0)
	    : is_input(_is_input)
	{
	    z_stream::next_in = new uint8_t();
	    z_stream::next_out = new uint8_t();
	    this->zalloc = Z_NULL;
	    this->zfree = Z_NULL;
	    this->opaque = Z_NULL;
	    if (is_input)
		{
		    z_stream::avail_in = 0;
		    z_stream::next_in = Z_NULL;
		    ret = inflateInit2(this, 15+32);
		}
	    else
		{
		    ret = deflateInit2(this, _level, Z_DEFLATED, 15+16, 8, Z_DEFAULT_STRATEGY);
		}
	    if (ret != Z_OK) {
		throw zException(this, ret);
	    }
	}
	~z_stream_wrapper()
	{
	    if (is_input)
		{
		    inflateEnd(this);
		}
	    else
		{
		    deflateEnd(this);
		}
	}

	int decompress(int _flags = Z_NO_FLUSH) override {
	    ret = inflate(this, _flags);
	    // process return code
	    if (ret != Z_OK && ret != Z_STREAM_END) throw zException(this, ret);
	    return ret;
	}
	int compress(int _flags = Z_NO_FLUSH) override {
            ret = deflate(this, _flags);
            if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR)
		throw zException(this, ret);
	    return ret;
	}
	bool stream_end() const override {
	    return this->ret == Z_STREAM_END;
	}
	bool done() const override {
	    return (this->ret == Z_BUF_ERROR || this->stream_end());
	}
	const uint8_t* next_in() override { return z_stream::next_in; }
	long avail_in() override { return z_stream::avail_in; }
	uint8_t* next_out() override { return z_stream::next_out; }
	long avail_out() override { return z_stream::avail_out; }
	void set_next_in(const unsigned char* in) override { z_stream::next_in = (unsigned char*)in; }
	void set_avail_in(long in) override { z_stream::avail_in = in; }
	void set_next_out(uint8_t* in) override { z_stream::next_out = in; }
	void set_avail_out(long in) override { z_stream::avail_out = in; }
    }; // class bz_stream_wrapper
} // namespace detail
} // namespace bxz

#endif
