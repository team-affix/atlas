#include <sstream>
#include <stdexcept>
#include <string>
#include <gtest/gtest.h>
#include "infrastructure/istream_uint64_extractor.hpp"

TEST(IstreamUint64ExtractorTest, ReadsLittleEndianQword) {
    istream_uint64_extractor sut;
    std::string bytes(8, '\0');
    bytes[0] = static_cast<char>(0x01);
    bytes[7] = static_cast<char>(0x08);
    std::istringstream in(bytes);
    EXPECT_EQ(sut.extract_uint64(in), 0x0800000000000001ull);
}

TEST(IstreamUint64ExtractorTest, TruncatedStreamThrows) {
    istream_uint64_extractor sut;
    std::istringstream in(std::string(7, '\0'));
    EXPECT_THROW(sut.extract_uint64(in), std::runtime_error);
}
