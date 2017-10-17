#include <gtest/gtest.h>
#include <png/reader.h>
#include <png/writer.h>
#include <algorithm>
#include <vector>

TEST(png, rgb) {
  const std::vector<unsigned char> bitmap = {
    0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,  // red    black  green
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // black  black  black
    0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,  // blue   black  white
  };
  png::writer writer;
  const auto image = writer.write("rgb", { reinterpret_cast<const char*>(bitmap.data()), bitmap.size() }, 3, 3, png::format::rgb);
  EXPECT_TRUE(png::reader::check(image));
  png::reader reader;
  std::size_t cx = 0;
  std::size_t cy = 0;
  auto format = png::format::rgb;
  const auto data = reader.read("rgb", image, cx, cy, format);
  EXPECT_EQ(cx, 3);
  EXPECT_EQ(cy, 3);
  EXPECT_EQ(format, png::format::rgb);
  EXPECT_EQ(data.size(), cx * cy * 3);
  EXPECT_TRUE(std::equal(data.begin(), data.end(), bitmap.begin(), bitmap.end(), [](auto a, auto b) { return static_cast<unsigned char>(a) == b; }));
}

TEST(png, rgba) {
  const std::vector<unsigned char> bitmap = {
    0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,  // red    black        green
    0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,  // black  transparent  black
    0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,  // blue   black        white
  };
  png::writer writer;
  const auto image = writer.write("rgba", { reinterpret_cast<const char*>(bitmap.data()), bitmap.size() }, 3, 3, png::format::rgba);
  EXPECT_TRUE(png::reader::check(image));
  png::reader reader;
  std::size_t cx = 0;
  std::size_t cy = 0;
  auto format = png::format::rgb;
  const auto data = reader.read("rgba", image, cx, cy, format);
  EXPECT_EQ(cx, 3);
  EXPECT_EQ(cy, 3);
  EXPECT_EQ(format, png::format::rgba);
  EXPECT_EQ(data.size(), cx * cy * 4);
  EXPECT_TRUE(std::equal(data.begin(), data.end(), bitmap.begin(), bitmap.end(), [](auto a, auto b) { return static_cast<unsigned char>(a) == b; }));
}
