// uniform_index_sample: uniform draw of an index in [0, count).

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstddef>
#include <limits>
#include "infrastructure/uniform_index_sample.hpp"

using ::testing::Return;

namespace {

struct MockEngine {
    using result_type = size_t;
    MOCK_METHOD(result_type, call, (), ());
    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return std::numeric_limits<size_t>::max(); }
    result_type operator()() { return call(); }
};

} // namespace

using test_uniform_index_sample_t = uniform_index_sample<MockEngine>;

struct UniformIndexSampleTest : public ::testing::Test {
    MockEngine engine;
    test_uniform_index_sample_t sample{engine};
};

TEST_F(UniformIndexSampleTest, SampleIndexOfOneIsAlwaysZero) {
    EXPECT_CALL(engine, call()).WillRepeatedly(Return(0u));
    EXPECT_EQ(sample.sample_index(1), 0u);
}

TEST_F(UniformIndexSampleTest, ConstantZeroEngineYieldsZero) {
    EXPECT_CALL(engine, call()).WillRepeatedly(Return(0u));
    EXPECT_EQ(sample.sample_index(4), 0u);
}
