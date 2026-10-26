#include <sstream>
#include <gtest/gtest.h>
#include "infrastructure/ostream_uint32_inserter.hpp"

TEST(OstreamUint32InserterTest, WritesFourLittleEndianBytes) {
    ostream_uint32_inserter sut;
    std::ostringstream out;
    sut.insert_uint32(out, 0x04030201u);
    EXPECT_EQ(static_cast<uint8_t>(out.str()[0]), 0x01u);
    EXPECT_EQ(static_cast<uint8_t>(out.str()[1]), 0x02u);
    EXPECT_EQ(static_cast<uint8_t>(out.str()[2]), 0x03u);
    EXPECT_EQ(static_cast<uint8_t>(out.str()[3]), 0x04u);
}
