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
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace png {

class reader {
public:
  reader() {
    png_ = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, [](png_structp ptr, png_const_charp msg) {
      throw std::runtime_error(msg ? msg : "png read error");
    }, [](png_structp ptr, png_const_charp msg) {
      if (msg) {
        std::cerr << msg << std::endl;
      }
    });
    if (!png_) {
      throw std::runtime_error("Could not initialize libpng for reading.");
    }
    info_ = png_create_info_struct(png_);
    if (!info_) {
      png_destroy_read_struct(&png_, nullptr, nullptr);
      throw std::runtime_error("Could not initialize libpng read info.");
    }
  }

  ~reader() {
    png_destroy_read_struct(&png_, &info_, nullptr);
  }

  std::string read(const std::string& name, std::string_view image, std::size_t& cx, std::size_t& cy, png::format& format) {
    // Read file and verify signature.
    const auto data = reinterpret_cast<png_const_bytep>(image.data());
    const auto size = static_cast<png_size_t>(image.size());
    if (!check(image)) {
      throw std::runtime_error("Invalid PNG signature image: " + name);
    }

    // Set read callback function.
    struct stream {
      std::string name;
      png_const_bytep beg;
      png_const_bytep end;
      png_const_bytep pos;
    } s = { name, data, data + size, data + signature_size };
    png_set_read_fn(png_, &s, [](png_structp ptr, png_bytep data, png_size_t size) {
      auto& self = *reinterpret_cast<stream*>(png_get_io_ptr(ptr));
      const auto beg = self.pos;
      const auto end = beg + size;
      if (end > self.end) {
        throw std::runtime_error("Could not read image data: " + self.name);
      }
      std::copy(beg, end, data);
      self.pos += size;
    });

    // Read image info.
    png_set_sig_bytes(png_, static_cast<int>(signature_size));
    png_read_info(png_, info_);
    const auto buffer_cx = png_get_image_width(png_, info_);
    const auto buffer_cy = png_get_image_height(png_, info_);
    cx = static_cast<std::size_t>(buffer_cx);
    cy = static_cast<std::size_t>(buffer_cy);

    // Convert channel size to one byte.
    if (png_get_bit_depth(png_, info_) < signature_size) {
      png_set_packing(png_);
    }

    // Convert transparency to alpha.
    if (png_get_valid(png_, info_, PNG_INFO_tRNS)) {
      png_set_tRNS_to_alpha(png_);
    }

    // Get format.
    switch (png_get_color_type(png_, info_)) {
    case PNG_COLOR_TYPE_GRAY:
      format = format::rgb;
      png_set_gray_to_rgb(png_);
      break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
      format = format::rgba;
      png_set_gray_to_rgb(png_);
      break;
    case PNG_COLOR_TYPE_PALETTE:
      format = format::rgb;
      png_set_expand(png_);
      break;
    case PNG_COLOR_TYPE_RGB:
      format = format::rgb;
      break;
    case PNG_COLOR_TYPE_RGBA:
      format = format::rgba;
      break;
    default:
      throw std::runtime_error("Unsupported color type in image: " + name);
    }

    // Disable interlacing.
    png_set_interlace_handling(png_);

    // Update image info.
    png_read_update_info(png_, info_);

    // Create image buffer.
    const auto bpp = png_get_rowbytes(png_, info_) / buffer_cx;
    std::string buffer;
    buffer.resize(buffer_cx * buffer_cy * bpp);

    // Read image.
    std::vector<png_bytep> rows;
    rows.resize(buffer_cy);
    for (png_uint_32 i = 0; i < buffer_cy; i++) {
      rows[i] = reinterpret_cast<png_bytep>(&buffer[i * buffer_cx * bpp]);
    }
    png_read_image(png_, &rows[0]);
    png_read_end(png_, nullptr);
    return buffer;
  }

  static bool check(std::string_view image) noexcept {
    if (image.size() > signature_size) {
#if PNG_LIBPNG_VER < 10600
      const auto data = reinterpret_cast<png_bytep>(const_cast<char*>(image.data()));
#else
      const auto data = reinterpret_cast<png_const_bytep>(image.data());
#endif
      return png_sig_cmp(data, 0, static_cast<png_size_t>(signature_size)) == 0;
    }
    return false;
  }

private:
  png_structp png_ = nullptr;
  png_infop info_ = nullptr;
};

}  // namespace png
