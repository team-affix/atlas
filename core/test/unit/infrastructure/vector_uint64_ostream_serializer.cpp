#include <sstream>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/vector_uint64_ostream_serializer.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::StrictMock;

namespace {

struct MockInsertUint64 {
    MOCK_METHOD(void, insert_uint64, (std::ostream&, uint64_t), (const));
};

using sut_t = vector_uint64_ostream_serializer<MockInsertUint64>;

struct VectorUint64OstreamSerializerTest : public ::testing::Test {
    StrictMock<MockInsertUint64> insert_uint64;
    sut_t sut{insert_uint64};
    std::ostringstream out;
};

}

TEST_F(VectorUint64OstreamSerializerTest, EmptyVectorWritesZeroCount) {
    EXPECT_CALL(insert_uint64, insert_uint64(_, 0u));
    sut.serialize_vector_uint64(out, {});
}

TEST_F(VectorUint64OstreamSerializerTest, WritesCountThenEachElement) {
    InSequence seq;
    EXPECT_CALL(insert_uint64, insert_uint64(_, 2u));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 5u));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 9u));
    sut.serialize_vector_uint64(out, {5, 9});
}
