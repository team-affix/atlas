#include <sstream>
#include <stdexcept>
#include <string>
#include <gtest/gtest.h>
#include "infrastructure/istream_uint8_extractor.hpp"

TEST(IstreamUint8ExtractorTest, ReadsSingleByte) {
    istream_uint8_extractor sut;
    std::istringstream in(std::string("\x7f", 1));
    EXPECT_EQ(sut.extract_uint8(in), 0x7fu);
}

TEST(IstreamUint8ExtractorTest, TruncatedStreamThrows) {
    istream_uint8_extractor sut;
    std::istringstream in;
    EXPECT_THROW(sut.extract_uint8(in), std::runtime_error);
}
