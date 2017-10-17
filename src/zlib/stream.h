// ISC License
//
// Copyright (c) 2017 Alexej Harm
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS AL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// Usage:
//
// std::string src = "data";
//
// std::string arc;
// zlib::deflate_stream ds;
// ds.process(src.data(), src.size(), [&](const char* data, std::size_t size) {
//   arc.append(data, size);
// }, true);
//
// std::string dst;
// zlib::inflate_stream is;
// is.process(arc.begin(), arc.end(), std::back_inserter(dst), true);
//

#pragma once
#include <zlib.h>
#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>

namespace zlib {

class error : public std::system_error {
public:
  using std::system_error::system_error;
};

class error_category_impl : public std::error_category {
public:
  const char* name() const noexcept override {
    return "zlib";
  }

  std::string message(int code) const override {
    if (const auto msg = zError(code)) {
      return msg;
    }
    return "Unknown error code: " + std::to_string(code);
  }
};

inline const std::error_category& error_category() noexcept {
  static error_category_impl error_category;
  return error_category;
}

inline std::error_code make_error(int code) noexcept {
  if (code == Z_ERRNO) {
    return std::error_code(errno, std::system_category());
  }
  return std::error_code(code, error_category());
}

enum class type {
  deflate,  // deflate stream
  inflate,  // inflate stream
};

enum class format {
  deflate,  // raw deflate without a header or trailer
  gzip,     // gzip header and trailer
  zlib,     // zlib header and trailer
};

template <type Type>
class stream {
public:
  // Initializes the stream.
  // format       : The (de)compression format.
  // level        : Compression level. Must be between 0 and 9.
  // window_bits  : Window size. Larger values result in better compression. Must be between 8 and 15.
  // memory_level : Memory level. Larger values result in better performance. Must be between 1 and 9.
  template <type V = type::deflate, typename = std::enable_if_t<Type == V, void>>
  stream(format format = format::deflate, int window_bits = 15, int level = -1, int memory_level = 8) {
    if (window_bits < 8 || window_bits > 15) {
      throw error(std::make_error_code(std::errc::invalid_argument), "Invalid window size");
    }
    if (level < -1 || level > 9) {
      throw error(std::make_error_code(std::errc::invalid_argument), "Invalid level");
    }
    if (memory_level < 1 || memory_level > 9) {
      throw error(std::make_error_code(std::errc::invalid_argument), "Invalid memory level");
    }
    switch (format) {
    case zlib::format::deflate: window_bits = -window_bits; break;
    case zlib::format::gzip: window_bits += 16; break;
    case zlib::format::zlib: break;
    default: throw error(std::make_error_code(std::errc::invalid_argument), "Invalid format"); break;
    }
    auto ev = deflateInit2(&stream_, level, Z_DEFLATED, window_bits, memory_level, Z_DEFAULT_STRATEGY);
    if (ev != Z_OK) {
      throw error(make_error(ev), "Deflate initialization failed");
    }
  }

  // Initializes the stream.
  // format       : The (de)compression format.
  // level        : Compression level. Must be between 0 and 9.
  // window_bits  : Window size. Larger values result in better compression. Must be between 8 and 15.
  // memory_level : Memory level. Larger values result in better performance. Must be between 1 and 9.
  template <type V = type::inflate, typename = std::enable_if_t<Type == V, void>>
  stream(format format = format::deflate, int window_bits = 15) {
    if (window_bits < 8 || window_bits > 15) {
      throw error(std::make_error_code(std::errc::invalid_argument), "Invalid window size");
    }
    switch (format) {
    case zlib::format::deflate: window_bits = -window_bits; break;
    case zlib::format::gzip: window_bits += 16; break;
    case zlib::format::zlib: break;
    default: throw error(std::make_error_code(std::errc::invalid_argument), "Invalid format"); break;
    }
    auto ev = inflateInit2(&stream_, window_bits);
    if (ev != Z_OK) {
      throw error(make_error(ev), "Inflate initialization failed");
    }
  }

  ~stream() {
    end(&stream_);
  }

  void reset() {
    if (finished_) {
      const auto ev = reset(&stream_);
      if (ev != Z_OK) {
        throw error(make_error(ev), "Reset failed");
      }
      finished_ = false;
    }
  }

  template <std::size_t Buffer = 8192, typename Handler>
  void process(const char* data, std::size_t size, const Handler& handler, bool finish = false) {
    using data_type = decltype(stream_.next_in);
    using size_type = decltype(stream_.avail_in);
    if (size == 0 && !finish) {
      return;
    }
    if (size > 0 && !data) {
      throw error(std::make_error_code(std::errc::invalid_argument), "Invalid data");
    }
    if (size > std::numeric_limits<size_type>::max()) {
      throw error(std::make_error_code(std::errc::invalid_argument), "Invalid data size");
    }
    if (finished_) {
      reset();
    }
    stream_.next_in = reinterpret_cast<data_type>(const_cast<char*>(data));
    stream_.avail_in = static_cast<size_type>(size);
    auto ev = Z_OK;
    std::array<char, Buffer> buffer;
    do {
      stream_.next_out = reinterpret_cast<data_type>(buffer.data());
      stream_.avail_out = static_cast<size_type>(buffer.size());
      ev = process(&stream_, finish ? Z_FINISH : Z_NO_FLUSH);
      const auto output_size = buffer.size() - stream_.avail_out;
      switch (ev) {
      case Z_OK: break;
      case Z_STREAM_END: break;
      case Z_BUF_ERROR:
        if (!finish) {
          return;
        }
        [[fallthrough]];
      default:
        throw error(make_error(ev), "Data processing failed");
        break;
      }
      if (output_size > 0) {
        handler(buffer.data(), output_size);
      }
    } while (ev != Z_STREAM_END && Type == type::deflate ? true : stream_.avail_in > 0);
    finished_ = finish && ev == Z_STREAM_END;
  }

  template <typename ContiguosIterator, typename OutputIterator>
  void process(ContiguosIterator begin, ContiguosIterator end, OutputIterator out, bool finish = false) {
    process(&*begin, static_cast<std::size_t>(std::distance(begin, end)), [&out](const char* data, std::size_t size) {
      std::copy_n(data, size, out);
    }, finish);
  }

  template <typename Handler>
  void finish(const Handler& handler) {
    process(nullptr, 0, handler, true);
  }

private:
  static int end(z_stream* stream) noexcept;
  static int reset(z_stream* stream) noexcept;
  static int process(z_stream* stream, int flush) noexcept;

  z_stream stream_ = {};
  bool finished_ = false;
};

template <>
inline int stream<type::deflate>::end(z_stream* stream) noexcept {
  return ::deflateEnd(stream);
}

template <>
inline int stream<type::inflate>::end(z_stream* stream) noexcept {
  return ::inflateEnd(stream);
}

template <>
inline int stream<type::deflate>::reset(z_stream* stream) noexcept {
  return ::deflateReset(stream);
}

template <>
inline int stream<type::inflate>::reset(z_stream* stream) noexcept {
  return ::inflateReset(stream);
}

template <>
inline int stream<type::deflate>::process(z_stream* stream, int flush) noexcept {
  return ::deflate(stream, flush);
}

template <>
inline int stream<type::inflate>::process(z_stream* stream, int flush) noexcept {
  return ::inflate(stream, flush);
}

using deflate_stream = stream<type::deflate>;
using inflate_stream = stream<type::inflate>;

}  // namespace zlib
