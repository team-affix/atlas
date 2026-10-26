#include <sstream>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/vector_uint64_istream_deserializer.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockExtractUint64 {
    MOCK_METHOD(uint64_t, extract_uint64, (std::istream&), (const));
};

using sut_t = vector_uint64_istream_deserializer<MockExtractUint64>;

struct VectorUint64IstreamDeserializerTest : public ::testing::Test {
    StrictMock<MockExtractUint64> extract_uint64;
    sut_t sut{extract_uint64};
    std::istringstream in;
};

}

TEST_F(VectorUint64IstreamDeserializerTest, ZeroCountReturnsEmptyVector) {
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(0u));
    EXPECT_THAT(sut.deserialize_vector_uint64(in), ::testing::IsEmpty());
}

TEST_F(VectorUint64IstreamDeserializerTest, ReadsCountThenEachElement) {
    InSequence seq;
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(2u));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(3u));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(7u));
    EXPECT_EQ(sut.deserialize_vector_uint64(in), std::vector<uint64_t>({3, 7}));
}
