#include <sstream>
#include <stdexcept>
#include <string>
#include <gtest/gtest.h>
#include "infrastructure/istream_fixed_bytes_extractor.hpp"

TEST(IstreamFixedBytesExtractorTest, ReadsExactByteCount) {
    istream_fixed_bytes_extractor sut;
    std::istringstream in("PUDM");
    char dest[4] = {};
    sut.extract_fixed_bytes(in, dest, 4);
    EXPECT_EQ(dest[0], 'P');
    EXPECT_EQ(dest[3], 'M');
}

TEST(IstreamFixedBytesExtractorTest, TruncatedStreamThrows) {
    istream_fixed_bytes_extractor sut;
    std::istringstream in("PU");
    char dest[4] = {};
    EXPECT_THROW(sut.extract_fixed_bytes(in, dest, 4), std::runtime_error);
}
