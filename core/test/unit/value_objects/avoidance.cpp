// avoidance: stores avoidance-clause members and watcher positions.

#include <gtest/gtest.h>
#include <vector>
#include "value_objects/avoidance.hpp"

struct AvoidanceTest : public ::testing::Test {
    goal_lineage goal0{nullptr, 0};
    resolution_lineage rl0{&goal0, 0};
    resolution_lineage rl1{&goal0, 1};
};

TEST_F(AvoidanceTest, DefaultConstructorFields) {
    const avoidance a;

    EXPECT_TRUE(a.members.empty());
    EXPECT_EQ(a.watcher_a_pos, 0u);
    EXPECT_EQ(a.watcher_b_pos, 1u);
}

TEST_F(AvoidanceTest, FullConstructorStoresMembersAndWatcherPositions) {
    const std::vector<const resolution_lineage*> members{&rl0, &rl1};
    const avoidance a{members, 3, 7};

    EXPECT_EQ(a.members, members);
    EXPECT_EQ(a.watcher_a_pos, 3u);
    EXPECT_EQ(a.watcher_b_pos, 7u);
}

TEST_F(AvoidanceTest, EqualityComparesAllFields) {
    const avoidance a{{&rl0, &rl1}, 2, 5};
    const avoidance same{{&rl0, &rl1}, 2, 5};
    const avoidance different_members{{&rl1}, 2, 5};
    const avoidance different_watcher_a{{&rl0, &rl1}, 9, 5};
    const avoidance different_watcher_b{{&rl0, &rl1}, 2, 9};

    EXPECT_EQ(a, same);
    EXPECT_NE(a, different_members);
    EXPECT_NE(a, different_watcher_a);
    EXPECT_NE(a, different_watcher_b);
}

TEST_F(AvoidanceTest, OrderingDistinguishesFields) {
    const avoidance low{{&rl0}, 0, 1};
    const avoidance high{{&rl0, &rl1}, 1, 2};

    EXPECT_LT(low, high);
    EXPECT_GT(high, low);
    EXPECT_EQ(low, low);
}
