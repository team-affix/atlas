#include <gtest/gtest.h>
#include "infrastructure/hex_uint64.hpp"

struct HexUint64Test : public ::testing::Test {
    hex_uint64 codec;
};

TEST_F(HexUint64Test, FormatZero) {
    EXPECT_EQ(codec.format_hex(0), "0");
}

TEST_F(HexUint64Test, FormatAndParseRoundTrip) {
    EXPECT_EQ(codec.format_hex(0xdeadbeefcafebabeull), "deadbeefcafebabe");
    EXPECT_EQ(codec.parse_hex("deadbeefcafebabe"), 0xdeadbeefcafebabeull);
}

TEST_F(HexUint64Test, ParseAcceptsUppercase) {
    EXPECT_EQ(codec.parse_hex("AB"), 0xabull);
}

TEST_F(HexUint64Test, ParseRejectsEmpty) {
    EXPECT_THROW(codec.parse_hex(""), std::runtime_error);
}

TEST_F(HexUint64Test, ParseRejectsInvalidDigit) {
    EXPECT_THROW(codec.parse_hex("12g"), std::runtime_error);
}
