// trail manages lexically scoped undo stacks of i_backtrackable mutations. Unit tests
// use MockBacktrackable to assert LIFO backtrack order across push/pop frames.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/trail.hpp"
#include "interfaces/i_backtrackable.hpp"

using ::testing::StrictMock;

struct MockBacktrackable : public i_backtrackable {
    MOCK_METHOD(void, backtrack, (), (override));
};

struct TrailTest : public ::testing::Test {
protected:
    trail t;
};

TEST_F(TrailTest, InitialDepthIsZero) {
    EXPECT_EQ(t.depth(), 0);
}

TEST_F(TrailTest, PushIncrementsDepth) {
    t.push();
    EXPECT_EQ(t.depth(), 1);
}

TEST_F(TrailTest, TwoPushesGiveDepth2) {
    t.push();
    t.push();
    EXPECT_EQ(t.depth(), 2);
}

TEST_F(TrailTest, PopDecrementsDepth) {
    t.push();
    t.pop();
    EXPECT_EQ(t.depth(), 0);
}

TEST_F(TrailTest, EmptyFramePopCallsNothing) {
    t.push();
    t.pop();
    EXPECT_EQ(t.depth(), 0);
}

TEST_F(TrailTest, LIFOUndoOrderWithinOneFrame) {
    auto m1 = std::make_unique<StrictMock<MockBacktrackable>>();
    auto m2 = std::make_unique<StrictMock<MockBacktrackable>>();
    ::testing::InSequence seq;
    EXPECT_CALL(*m2, backtrack()).Times(1);
    EXPECT_CALL(*m1, backtrack()).Times(1);

    t.push();
    t.log(std::move(m1));
    t.log(std::move(m2));
    t.pop();
}

TEST_F(TrailTest, TwoFramesFirstPopUndoesOnlyFrame2Entry) {
    auto m1 = std::make_unique<StrictMock<MockBacktrackable>>();
    auto m2 = std::make_unique<StrictMock<MockBacktrackable>>();
    ::testing::InSequence seq;
    EXPECT_CALL(*m2, backtrack()).Times(1);

    t.push();
    t.log(std::move(m1));
    t.push();
    t.log(std::move(m2));
    t.pop();
    EXPECT_EQ(t.depth(), 1);
}

TEST_F(TrailTest, TwoFramesSecondPopUndoesFrame1Entry) {
    auto m1a = std::make_unique<StrictMock<MockBacktrackable>>();
    auto m1b = std::make_unique<StrictMock<MockBacktrackable>>();
    auto m2 = std::make_unique<StrictMock<MockBacktrackable>>();
    ::testing::InSequence seq;
    EXPECT_CALL(*m2, backtrack()).Times(1);
    EXPECT_CALL(*m1b, backtrack()).Times(1);
    EXPECT_CALL(*m1a, backtrack()).Times(1);

    t.push();
    t.log(std::move(m1a));
    t.log(std::move(m1b));
    t.push();
    t.log(std::move(m2));
    EXPECT_EQ(t.depth(), 2);

    t.pop();
    EXPECT_EQ(t.depth(), 1);

    t.pop();
    EXPECT_EQ(t.depth(), 0);
}
