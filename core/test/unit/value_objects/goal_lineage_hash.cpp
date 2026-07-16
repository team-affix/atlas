// goal_lineage_hash: hashes goal_lineage for unordered containers.

#include <gtest/gtest.h>
#include <unordered_set>
#include "value_objects/goal_lineage_hash.hpp"

struct GoalLineageHashTest : public ::testing::Test {
    goal_lineage_hash hasher;

    resolution_lineage parent0{nullptr, 0};
    resolution_lineage parent1{nullptr, 1};
};

TEST_F(GoalLineageHashTest, SameParentAndIdxHashesEqual) {
    const goal_lineage gl0{&parent0, 2};
    const goal_lineage gl0_copy{&parent0, 2};
    EXPECT_EQ(hasher(gl0), hasher(gl0_copy));
}

TEST_F(GoalLineageHashTest, DifferentIdxHashesDiffer) {
    const goal_lineage gl0{&parent0, 0};
    const goal_lineage gl1{&parent0, 1};
    EXPECT_NE(hasher(gl0), hasher(gl1));
}

TEST_F(GoalLineageHashTest, DifferentParentPointerHashesDiffer) {
    const goal_lineage gl0{&parent0, 0};
    const goal_lineage gl1{&parent1, 0};
    EXPECT_NE(hasher(gl0), hasher(gl1));
}

TEST_F(GoalLineageHashTest, NullParentHashesEqual) {
    const goal_lineage gl0{nullptr, 0};
    const goal_lineage gl1{nullptr, 0};
    EXPECT_EQ(hasher(gl0), hasher(gl1));
}

TEST_F(GoalLineageHashTest, WorksAsUnorderedSetKey) {
    const goal_lineage gl0{&parent0, 0};
    const goal_lineage gl1{&parent0, 1};
    std::unordered_set<goal_lineage, goal_lineage_hash> keys;

    keys.insert(gl0);
    keys.insert(gl1);

    EXPECT_EQ(keys.size(), 2u);
    EXPECT_TRUE(keys.contains(gl0));
    EXPECT_TRUE(keys.contains(gl1));
}
