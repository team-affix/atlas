// resolution_lineage_hash: hashes resolution_lineage for unordered containers.

#include <gtest/gtest.h>
#include <unordered_set>
#include "value_objects/resolution_lineage_hash.hpp"

struct ResolutionLineageHashTest : public ::testing::Test {
    resolution_lineage_hash hasher;

    goal_lineage parent0{nullptr, 0};
    goal_lineage parent1{nullptr, 1};
};

TEST_F(ResolutionLineageHashTest, SameParentAndRuleIdHashesEqual) {
    const resolution_lineage rl0{&parent0, 2};
    const resolution_lineage rl0_copy{&parent0, 2};
    EXPECT_EQ(hasher(rl0), hasher(rl0_copy));
}

TEST_F(ResolutionLineageHashTest, DifferentRuleIdHashesDiffer) {
    const resolution_lineage rl0{&parent0, 0};
    const resolution_lineage rl1{&parent0, 1};
    EXPECT_NE(hasher(rl0), hasher(rl1));
}

TEST_F(ResolutionLineageHashTest, DifferentParentPointerHashesDiffer) {
    const resolution_lineage rl0{&parent0, 0};
    const resolution_lineage rl1{&parent1, 0};
    EXPECT_NE(hasher(rl0), hasher(rl1));
}

TEST_F(ResolutionLineageHashTest, NullParentHashesEqual) {
    const resolution_lineage rl0{nullptr, 0};
    const resolution_lineage rl1{nullptr, 0};
    EXPECT_EQ(hasher(rl0), hasher(rl1));
}

TEST_F(ResolutionLineageHashTest, WorksAsUnorderedSetKey) {
    const resolution_lineage rl0{&parent0, 0};
    const resolution_lineage rl1{&parent0, 1};
    std::unordered_set<resolution_lineage, resolution_lineage_hash> keys;

    keys.insert(rl0);
    keys.insert(rl1);

    EXPECT_EQ(keys.size(), 2u);
    EXPECT_TRUE(keys.contains(rl0));
    EXPECT_TRUE(keys.contains(rl1));
}
