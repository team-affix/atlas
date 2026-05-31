// Deactivated candidate memory: tracks resolution lineages ruled out for reactivation.
// contains/insert/clear must stay consistent; clear must empty the set.

#include <gtest/gtest.h>
#include "infrastructure/deactivated_candidate_memory.hpp"
#include "value_objects/lineage.hpp"

struct DeactivatedCandidateMemoryTest : public ::testing::Test {
    deactivated_candidate_memory memory;
    goal_lineage parent{nullptr, 0};
    resolution_lineage rl{&parent, 0};
};

TEST_F(DeactivatedCandidateMemoryTest, NotContainsInitially) {
    EXPECT_FALSE(memory.contains(&rl));
}

TEST_F(DeactivatedCandidateMemoryTest, InsertMakesContainsTrue) {
    memory.insert(&rl);
    EXPECT_TRUE(memory.contains(&rl));
}

TEST_F(DeactivatedCandidateMemoryTest, ClearRemovesAll) {
    memory.insert(&rl);
    memory.clear();
    EXPECT_FALSE(memory.contains(&rl));
}

TEST_F(DeactivatedCandidateMemoryTest, DuplicateInsertIsIdempotent) {
    memory.insert(&rl);
    memory.insert(&rl);
    EXPECT_TRUE(memory.contains(&rl));
}

TEST_F(DeactivatedCandidateMemoryTest, DistinctLineagesTrackedIndependently) {
    goal_lineage parent2{nullptr, 1};
    resolution_lineage rl2{&parent2, 0};
    memory.insert(&rl);
    EXPECT_FALSE(memory.contains(&rl2));
    memory.insert(&rl2);
    EXPECT_TRUE(memory.contains(&rl));
    EXPECT_TRUE(memory.contains(&rl2));
}
