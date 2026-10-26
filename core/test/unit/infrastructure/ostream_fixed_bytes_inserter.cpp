#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include "infrastructure/ostream_fixed_bytes_inserter.hpp"

TEST(OstreamFixedBytesInserterTest, WritesExactByteSpan) {
    ostream_fixed_bytes_inserter sut;
    std::ostringstream out;
    const char bytes[3] = {'P', 'U', 'D'};
    sut.insert_fixed_bytes(out, bytes, 3);
    EXPECT_EQ(out.str(), std::string("PUD"));
}
