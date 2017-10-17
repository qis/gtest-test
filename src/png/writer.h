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

#pragma once
#include <png/common.h>
#include <string>
#include <string_view>

namespace png {

class writer {
public:
  writer() {
    png_ = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, [](png_structp ptr, png_const_charp msg) {
      throw std::runtime_error(msg ? msg : "png write error");
    }, [](png_structp ptr, png_const_charp msg) {
      if (msg) {
        std::cerr << msg << std::endl;
      }
    });
    if (!png_) {
      throw std::runtime_error("Could not initialize libpng for writing.");
    }
    info_ = png_create_info_struct(png_);
    if (!info_) {
      png_destroy_write_struct(&png_, nullptr);
      throw std::runtime_error("Could not initialize libpng write info.");
    }
  }

  ~writer() {
    png_destroy_write_struct(&png_, &info_);
  }

  std::string write(const std::string& name, std::string_view image, std::size_t cx, std::size_t cy, png::format format) {
    std::string buffer;
    struct stream {
      std::string name;
      std::string& buffer;
    } s = { name, buffer };
    png_set_write_fn(png_, &s, [](png_structp ptr, png_bytep data, png_size_t size) {
      auto& self = *reinterpret_cast<stream*>(png_get_io_ptr(ptr));
      self.buffer.append(reinterpret_cast<const char*>(data), size);
    }, [](png_structp ptr) {
    });
    const auto buffer_cx = static_cast<png_uint_32>(cx);
    const auto buffer_cy = static_cast<png_uint_32>(cy);
    const auto data = reinterpret_cast<png_const_bytep>(image.data());
    const auto type = format == png::format::rgb ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA;
    const auto size = format == png::format::rgb ? 3 : 4;
    png_set_IHDR(png_, info_, buffer_cx, buffer_cy, signature_size, type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_, info_);
    for (std::size_t i = 0; i < cy; i++) {
#if PNG_LIBPNG_VER < 10600
      png_write_row(png_, const_cast<png_bytep>(data + cx * i * size));
#else
      png_write_row(png_, data + cx * i * size);
#endif
    }
    png_write_end(png_, nullptr);
    return buffer;
  }

private:
  png_structp png_ = nullptr;
  png_infop info_ = nullptr;
};

}  // namespace png
