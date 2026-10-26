#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include "infrastructure/ostream_uint8_inserter.hpp"

TEST(OstreamUint8InserterTest, WritesSingleLittleEndianByte) {
    ostream_uint8_inserter sut;
    std::ostringstream out;
    sut.insert_uint8(out, 0xabu);
    EXPECT_EQ(out.str(), std::string("\xab", 1));
}
