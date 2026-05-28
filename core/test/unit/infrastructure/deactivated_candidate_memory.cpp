// Deactivated candidate memory: tracks resolution lineages ruled out for reactivation.
// contains/insert/clear must stay consistent; clear must empty the set.

#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/deactivated_candidate_memory.hpp"
#include "../../../core/hpp/value_objects/lineage.hpp"

struct DeactivatedCandidateMemoryTest : public ::testing::Test {
    deactivated_candidate_memory memory;
    expr goal_e{expr::var{0}};
    expr head{expr::var{1}};
    rule idx{&head, {}};
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
