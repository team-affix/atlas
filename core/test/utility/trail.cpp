#include <gtest/gtest.h>
#include "../../../core/hpp/utility/trail.hpp"
#include <vector>

namespace {
    struct recorder : i_backtrackable {
        std::vector<int>& order;
        int id;
        recorder(std::vector<int>& o, int id) : order(o), id(id) {}
        void backtrack() override { order.push_back(id); }
    };
}

class TrailTest : public ::testing::Test {
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
    std::vector<int> order;
    t.push();
    t.pop();
    EXPECT_TRUE(order.empty());
    EXPECT_EQ(t.depth(), 0);
}

TEST_F(TrailTest, LIFOUndoOrderWithinOneFrame) {
    std::vector<int> order;
    t.push();
    t.log(std::make_unique<recorder>(order, 1));
    t.log(std::make_unique<recorder>(order, 2));
    t.pop();
    EXPECT_EQ(order, (std::vector<int>{2, 1}));
}

TEST_F(TrailTest, TwoFramesFirstPopUndoesOnlyFrame2Entry) {
    std::vector<int> order;
    t.push();
    t.log(std::make_unique<recorder>(order, 1));
    t.push();
    t.log(std::make_unique<recorder>(order, 2));
    t.pop();
    EXPECT_EQ(order, (std::vector<int>{2}));
    EXPECT_EQ(t.depth(), 1);
}

TEST_F(TrailTest, TwoFramesSecondPopUndoesFrame1Entry) {
    std::vector<int> order;
    t.push();
    t.log(std::make_unique<recorder>(order, 1));
    t.push();
    t.log(std::make_unique<recorder>(order, 2));
    t.pop();
    order.clear();
    t.pop();
    EXPECT_EQ(order, (std::vector<int>{1}));
    EXPECT_EQ(t.depth(), 0);
}
