#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/lineage_pool.hpp"

class LineagePoolTest : public ::testing::Test {
protected:
    lineage_pool pool;
};

// ---------------------------------------------------------------------------
// Interning / identity
// ---------------------------------------------------------------------------

TEST_F(LineagePoolTest, GoalLineageInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.goal(nullptr, 0), pool.goal(nullptr, 0));
}

TEST_F(LineagePoolTest, DifferentGoalIndicesReturnDifferentPointers) {
    EXPECT_NE(pool.goal(nullptr, 0), pool.goal(nullptr, 1));
}

TEST_F(LineagePoolTest, ResolutionLineageInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.resolution(nullptr, 0), pool.resolution(nullptr, 0));
}

TEST_F(LineagePoolTest, DifferentResolutionIndicesReturnDifferentPointers) {
    EXPECT_NE(pool.resolution(nullptr, 0), pool.resolution(nullptr, 1));
}

TEST_F(LineagePoolTest, GoalWithParentInternedTwiceReturnsSamePointer) {
    const resolution_lineage* res = pool.resolution(nullptr, 0);
    EXPECT_EQ(pool.goal(res, 0), pool.goal(res, 0));
}

TEST_F(LineagePoolTest, GoalWithDifferentParentsReturnDifferentPointers) {
    const resolution_lineage* res0 = pool.resolution(nullptr, 0);
    const resolution_lineage* res1 = pool.resolution(nullptr, 1);
    EXPECT_NE(pool.goal(res0, 0), pool.goal(res1, 0));
}

// ---------------------------------------------------------------------------
// pin propagates up the ancestry chain
//
// Chain used in all pin tests:
//   res0 (nullptr parent)
//     └── goal0 (parent = res0, idx 0)
//           └── res1 (parent = goal0, idx 0)
//                 └── goal1 (parent = res1, idx 0)
// ---------------------------------------------------------------------------

TEST_F(LineagePoolTest, PinGoalPinsItsResolutionParent) {
    const resolution_lineage* res0  = pool.resolution(nullptr, 0);
    const goal_lineage*       goal0 = pool.goal(res0, 0);

    pool.pin(goal0);
    pool.trim();

    EXPECT_EQ(pool.goal(res0, 0),        goal0); // goal0 survived
    EXPECT_EQ(pool.resolution(nullptr, 0), res0); // res0 (ancestor) survived
}

TEST_F(LineagePoolTest, PinLeafGoalPinsEntireChainToRoot) {
    const resolution_lineage* res0  = pool.resolution(nullptr, 0);
    const goal_lineage*       goal0 = pool.goal(res0, 0);
    const resolution_lineage* res1  = pool.resolution(goal0, 0);
    const goal_lineage*       goal1 = pool.goal(res1, 0);

    pool.pin(goal1);
    pool.trim();

    EXPECT_EQ(pool.resolution(nullptr, 0), res0);
    EXPECT_EQ(pool.goal(res0, 0),          goal0);
    EXPECT_EQ(pool.resolution(goal0, 0),   res1);
    EXPECT_EQ(pool.goal(res1, 0),          goal1);
}

TEST_F(LineagePoolTest, PinAlreadyPinnedAncestorStopsTraversal) {
    const resolution_lineage* res0  = pool.resolution(nullptr, 0);
    const goal_lineage*       goal0 = pool.goal(res0, 0);
    const resolution_lineage* res1  = pool.resolution(goal0, 0);
    const goal_lineage*       goal1 = pool.goal(res1, 0);

    pool.pin(goal0);  // pins res0 + goal0
    pool.pin(goal1);  // pins res1 + goal1; short-circuits at res0 (already pinned)
    pool.trim();

    EXPECT_EQ(pool.resolution(nullptr, 0), res0);
    EXPECT_EQ(pool.goal(res0, 0),          goal0);
    EXPECT_EQ(pool.resolution(goal0, 0),   res1);
    EXPECT_EQ(pool.goal(res1, 0),          goal1);
}

// ---------------------------------------------------------------------------
// trim removes unpinned, keeps pinned
// ---------------------------------------------------------------------------

TEST_F(LineagePoolTest, TrimKeepsPinned) {
    const goal_lineage* g0 = pool.goal(nullptr, 0);
    pool.goal(nullptr, 1);

    pool.pin(g0);
    pool.trim();

    EXPECT_EQ(pool.goal(nullptr, 0), g0);  // pinned: same pointer
}

TEST_F(LineagePoolTest, TrimIsIdempotentForPinnedItems) {
    const goal_lineage* g = pool.goal(nullptr, 0);
    pool.pin(g);
    pool.trim();
    pool.trim();
    EXPECT_EQ(pool.goal(nullptr, 0), g);
}

// ---------------------------------------------------------------------------
// import
// ---------------------------------------------------------------------------

TEST_F(LineagePoolTest, ImportNullGoalReturnsNull) {
    EXPECT_EQ(pool.import(static_cast<const goal_lineage*>(nullptr)), nullptr);
}

TEST_F(LineagePoolTest, ImportNullResolutionReturnsNull) {
    EXPECT_EQ(pool.import(static_cast<const resolution_lineage*>(nullptr)), nullptr);
}

TEST_F(LineagePoolTest, ImportGoalLineageFromExternalPointers) {
    resolution_lineage raw_res{nullptr, 0};
    goal_lineage       raw_goal{&raw_res, 0};

    const goal_lineage* p = pool.import(&raw_goal);
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(pool.import(&raw_goal), p); // idempotent
}

TEST_F(LineagePoolTest, ImportResolutionRecursivelyImportsParent) {
    goal_lineage       raw_parent{nullptr, 0};
    resolution_lineage raw_leaf{&raw_parent, 0};

    const resolution_lineage* leaf = pool.import(&raw_leaf);
    EXPECT_NE(leaf, nullptr);

    const goal_lineage* parent = pool.goal(nullptr, 0);
    EXPECT_EQ(leaf->parent, parent); // parent was also interned into the pool
    EXPECT_EQ(pool.import(&raw_parent), parent); // re-import of root returns same ptr
}
