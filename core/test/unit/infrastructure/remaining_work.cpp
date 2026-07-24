// remaining_work: sim-scoped remaining-work register. Invariants: starts at zero,
// add/subtract adjust, clear resets.

#include <gtest/gtest.h>
#include "infrastructure/remaining_work.hpp"

struct RemainingWorkTest : public ::testing::Test {
    remaining_work store;

    static constexpr double kWorkA = 0.25;
    static constexpr double kWorkB = 0.75;
};

TEST_F(RemainingWorkTest, StartsAtZero) {
    EXPECT_DOUBLE_EQ(store.get(), 0.0);
}

TEST_F(RemainingWorkTest, AddIncreasesRegister) {
    store.add(kWorkA);
    EXPECT_DOUBLE_EQ(store.get(), kWorkA);
    store.add(kWorkB);
    EXPECT_DOUBLE_EQ(store.get(), kWorkA + kWorkB);
}

TEST_F(RemainingWorkTest, SubtractDecreasesRegister) {
    store.add(kWorkA + kWorkB);
    store.subtract(kWorkA);
    EXPECT_DOUBLE_EQ(store.get(), kWorkB);
}

TEST_F(RemainingWorkTest, ClearResetsRegister) {
    store.add(kWorkA);
    store.clear();
    EXPECT_DOUBLE_EQ(store.get(), 0.0);
}
