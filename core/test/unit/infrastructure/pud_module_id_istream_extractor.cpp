#include <istream>
#include <sstream>
#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_module_id_istream_extractor.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArrayArgument;
using ::testing::StrictMock;

namespace {

struct MockExtractFixedBytes {
    MOCK_METHOD(void, extract_fixed_bytes, (std::istream&, char*, size_t), (const));
};
struct MockExtractUint64 {
    MOCK_METHOD(uint64_t, extract_uint64, (std::istream&), (const));
};

struct PudModuleIdIstreamExtractorTest : public ::testing::Test {
    StrictMock<MockExtractFixedBytes> extract_fixed_bytes;
    StrictMock<MockExtractUint64> extract_uint64;
    pud_module_id_istream_extractor<StrictMock<MockExtractFixedBytes>, StrictMock<MockExtractUint64>> extractor{
        extract_fixed_bytes, extract_uint64};
    std::istringstream in;
    const char magic[4] = {'P', 'U', 'D', 'M'};
    const char wrong_magic[4] = {'X', 'Y', 'Z', 'W'};
};

}

TEST_F(PudModuleIdIstreamExtractorTest, TheMagicIsCheckedThenTheModuleIdIsRead) {
    InSequence order;
    EXPECT_CALL(extract_fixed_bytes, extract_fixed_bytes(_, _, 4)).WillOnce(SetArrayArgument<1>(magic, magic + 4));
    EXPECT_CALL(extract_uint64, extract_uint64(_)).WillOnce(Return(0xbe72u));
    EXPECT_EQ(extractor.extract_module_id(in), 0xbe72u);
}

TEST_F(PudModuleIdIstreamExtractorTest, AWrongMagicThrowsBeforeTheIdIsRead) {
    EXPECT_CALL(extract_fixed_bytes, extract_fixed_bytes(_, _, 4))
        .WillOnce(SetArrayArgument<1>(wrong_magic, wrong_magic + 4));
    EXPECT_THROW(extractor.extract_module_id(in), std::runtime_error);
}
