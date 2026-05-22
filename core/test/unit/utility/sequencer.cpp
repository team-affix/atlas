#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/utility/sequencer.hpp"
#include "../../../core/hpp/utility/i_trail.hpp"

using ::testing::_;
using ::testing::NiceMock;

struct MockTrail : public i_trail {
    MOCK_METHOD(void, push, (), (override));
    MOCK_METHOD(void, pop, (), (override));
    MOCK_METHOD(void, log, ((std::unique_ptr<i_backtrackable>)), (override));
};

struct SequencerTest : public ::testing::Test {
protected:
    NiceMock<MockTrail> trail;
    sequencer<int> seq{trail};
};

TEST_F(SequencerTest, NextReturns0Then1Then2InOrder) {
    EXPECT_CALL(trail, log(_)).Times(3);

    EXPECT_EQ(seq.next(), 0);
    EXPECT_EQ(seq.next(), 1);
    EXPECT_EQ(seq.next(), 2);
}
