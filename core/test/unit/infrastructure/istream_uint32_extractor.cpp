#include <sstream>
#include <stdexcept>
#include <string>
#include <gtest/gtest.h>
#include "infrastructure/istream_uint32_extractor.hpp"

TEST(IstreamUint32ExtractorTest, ReadsLittleEndianWord) {
    istream_uint32_extractor sut;
    std::string bytes;
    bytes.push_back(static_cast<char>(0x01));
    bytes.push_back(static_cast<char>(0x02));
    bytes.push_back(static_cast<char>(0x03));
    bytes.push_back(static_cast<char>(0x04));
    std::istringstream in(bytes);
    EXPECT_EQ(sut.extract_uint32(in), 0x04030201u);
}

TEST(IstreamUint32ExtractorTest, TruncatedStreamThrows) {
    istream_uint32_extractor sut;
    std::istringstream in(std::string(3, '\0'));
    EXPECT_THROW(sut.extract_uint32(in), std::runtime_error);
}
