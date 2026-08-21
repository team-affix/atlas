// dbuct_remaining_work: add/subtract with framed undo on pop_frame.

#include <gtest/gtest.h>
#include "infrastructure/dbuct_remaining_work.hpp"

struct DbuctRemainingWorkTest : public ::testing::Test {
    dbuct_remaining_work store;

    static constexpr double kWorkA = 0.25;
    static constexpr double kWorkB = 0.5;
};

TEST_F(DbuctRemainingWorkTest, StartsAtZero) {
    EXPECT_DOUBLE_EQ(store.get(), 0.0);
}

TEST_F(DbuctRemainingWorkTest, AddAndSubtractAdjustRegister) {
    store.add(kWorkA);
    store.add(kWorkB);
    store.subtract(kWorkA);
    EXPECT_DOUBLE_EQ(store.get(), kWorkB);
}

TEST_F(DbuctRemainingWorkTest, PopFrameUndoesAdd) {
    store.add(kWorkA);
    store.push_frame();
    store.add(kWorkB);
    EXPECT_DOUBLE_EQ(store.get(), kWorkA + kWorkB);
    store.pop_frame();
    EXPECT_DOUBLE_EQ(store.get(), kWorkA);
}

TEST_F(DbuctRemainingWorkTest, PopFrameUndoesSubtract) {
    store.add(kWorkA + kWorkB);
    store.push_frame();
    store.subtract(kWorkB);
    EXPECT_DOUBLE_EQ(store.get(), kWorkA);
    store.pop_frame();
    EXPECT_DOUBLE_EQ(store.get(), kWorkA + kWorkB);
}

TEST_F(DbuctRemainingWorkTest, GetAfterUndoRestored) {
    store.add(kWorkA);
    store.push_frame();
    store.add(kWorkB);
    store.subtract(kWorkA);
    store.pop_frame();
    EXPECT_DOUBLE_EQ(store.get(), kWorkA);
}

TEST_F(DbuctRemainingWorkTest, SubtractBelowZeroYieldsNegativeBalance) {
    // The register is a signed balance, not a clamped counter: quell_reward
    // negates it, so clamping at zero would turn a work overdraft into a
    // maximal reward and steer the search straight at the broken branch.
    store.add(kWorkA);
    store.subtract(kWorkA + kWorkB);
    EXPECT_DOUBLE_EQ(store.get(), -kWorkB);

    store.push_frame();
    store.subtract(kWorkA);
    EXPECT_DOUBLE_EQ(store.get(), -kWorkB - kWorkA);
    store.pop_frame();
    EXPECT_DOUBLE_EQ(store.get(), -kWorkB);
}

TEST_F(DbuctRemainingWorkTest, ThreeNestedFramesUndoInReverseOrder) {
    store.add(kWorkA);

    store.push_frame();
    store.add(kWorkB);
    store.push_frame();
    store.subtract(kWorkA);
    store.push_frame();
    store.add(kWorkA + kWorkB);
    EXPECT_DOUBLE_EQ(store.get(), 2.0 * kWorkB + kWorkA);

    store.pop_frame();
    EXPECT_DOUBLE_EQ(store.get(), kWorkB);
    store.pop_frame();
    EXPECT_DOUBLE_EQ(store.get(), kWorkA + kWorkB);
    store.pop_frame();
    EXPECT_DOUBLE_EQ(store.get(), kWorkA);
}
