#include <gtest/gtest.h>
#include <zlib/stream.h>
#include <iterator>
#include <string>

namespace test {

void zlib(zlib::format format) {
  const std::string src = "Yog-Sothoth knows the gate. Yog-Sothoth is the gate. Yog-Sothoth is the key and guardian of the gate.";

  std::string arc;
  zlib::deflate_stream ds(format);
  ds.process(src.data(), src.size(), [&](const char* data, std::size_t size) {
    arc.append(data, size);
  }, true);

  EXPECT_NE(src, arc);

  std::string dst;
  zlib::inflate_stream is(format);
  is.process(arc.begin(), arc.end(), std::back_inserter(dst), true);

  EXPECT_EQ(src, dst);
}

}  // namespace test

TEST(zlib, deflate) {
  test::zlib(zlib::format::deflate);
}

TEST(zlib, gzip) {
  test::zlib(zlib::format::gzip);
}

TEST(zlib, zlib) {
  test::zlib(zlib::format::zlib);
}
