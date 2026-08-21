// goal_lineage / resolution_lineage: the defaulted operator<=>.
//
// Only their hashes are covered (value_objects/goal_lineage_hash.cpp,
// resolution_lineage_hash.cpp). Lineages are the key of nearly every store in
// the solver, so an ordering that conflated two of them would hand sibling
// branches the same goal expression and bindings.

#include <array>
#include <gtest/gtest.h>
#include "value_objects/lineage.hpp"

struct LineageTest : public ::testing::Test {
    goal_lineage root{nullptr, 0};
    // Held in one array so the two parent pointers have a defined relative order.
    std::array<resolution_lineage, 2> resolutions{
        resolution_lineage{&root, 0}, resolution_lineage{&root, 1}};
};

TEST_F(LineageTest, GoalAndResolutionLineageOrderByParentThenIdx) {
    // resolution_lineage: sharing a parent goal, ordering falls to the rule id.
    // These are the competing candidate rules for one goal, so they must stay
    // distinct or CDCL eliminates the wrong branch.
    EXPECT_EQ(resolutions[0], (resolution_lineage{&root, 0}));
    EXPECT_NE(resolutions[0], resolutions[1]);
    EXPECT_LT(resolutions[0], resolutions[1]);

    // goal_lineage: sharing a parent resolution, ordering falls to the subgoal index.
    const goal_lineage subgoal_0{&resolutions[0], 0};
    const goal_lineage subgoal_1{&resolutions[0], 1};
    EXPECT_EQ(subgoal_0, (goal_lineage{&resolutions[0], 0}));
    EXPECT_LT(subgoal_0, subgoal_1);

    // The parent dominates the index: the same subgoal position under two
    // different resolutions of the same goal stays apart.
    const goal_lineage sibling_0{&resolutions[1], 0};
    EXPECT_NE(subgoal_0, sibling_0);
    EXPECT_LT(subgoal_0, sibling_0);
    EXPECT_LT(subgoal_1, sibling_0);
}
