#include <sstream>
#include <gtest/gtest.h>
#include "infrastructure/ostream_uint64_inserter.hpp"

TEST(OstreamUint64InserterTest, WritesEightLittleEndianBytes) {
    ostream_uint64_inserter sut;
    std::ostringstream out;
    sut.insert_uint64(out, 0x0807060504030201ull);
    EXPECT_EQ(out.str().size(), 8u);
    EXPECT_EQ(static_cast<uint8_t>(out.str()[0]), 0x01u);
    EXPECT_EQ(static_cast<uint8_t>(out.str()[7]), 0x08u);
}
