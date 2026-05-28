// lineage_pool interns goal and resolution lineages, pins ancestry for retention, and
// trims unpinned nodes. Unit tests cover intern identity, pin propagation, trim GC,
// and import without external mocks (no i_* dependencies).

#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/lineage_pool.hpp"

struct LineagePoolTest : public ::testing::Test {
protected:
    lineage_pool pool;
    expr goal_expr0{expr::var{0}};
    expr goal_expr1{expr::var{1}};
    expr rule_head0{expr::var{10}};
    expr rule_head1{expr::var{11}};
    rule rule_idx0{&rule_head0, {}};
    rule rule_idx1{&rule_head1, {}};
};

// ---------------------------------------------------------------------------
// Interning / identity
// ---------------------------------------------------------------------------

TEST_F(LineagePoolTest, GoalLineageInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make(nullptr, &goal_expr0), pool.make(nullptr, &goal_expr0));
}

TEST_F(LineagePoolTest, DifferentGoalIndicesReturnDifferentPointers) {
    EXPECT_NE(pool.make(nullptr, &goal_expr0), pool.make(nullptr, &goal_expr1));
}

TEST_F(LineagePoolTest, ResolutionLineageInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make(nullptr, &rule_idx0), pool.make(nullptr, &rule_idx0));
}

TEST_F(LineagePoolTest, DifferentResolutionIndicesReturnDifferentPointers) {
    EXPECT_NE(pool.make(nullptr, &rule_idx0), pool.make(nullptr, &rule_idx1));
}

TEST_F(LineagePoolTest, GoalWithParentInternedTwiceReturnsSamePointer) {
    const resolution_lineage* res = pool.make(nullptr, &rule_idx0);
    EXPECT_EQ(pool.make(res, &goal_expr0), pool.make(res, &goal_expr0));
}

TEST_F(LineagePoolTest, GoalWithDifferentParentsReturnDifferentPointers) {
    const resolution_lineage* res0 = pool.make(nullptr, &rule_idx0);
    const resolution_lineage* res1 = pool.make(nullptr, &rule_idx1);
    EXPECT_NE(pool.make(res0, &goal_expr0), pool.make(res1, &goal_expr0));
}

// ---------------------------------------------------------------------------
// pin propagates up the ancestry chain
//
// Chain used in all pin tests:
//   res0 (nullptr parent)
//     └── goal0 (parent = res0, idx goal_expr0)
//           └── res1 (parent = goal0, idx rule_idx0)
//                 └── goal1 (parent = res1, idx goal_expr0)
// ---------------------------------------------------------------------------

TEST_F(LineagePoolTest, PinGoalPinsItsResolutionParent) {
    const resolution_lineage* res0  = pool.make(nullptr, &rule_idx0);
    const goal_lineage*       goal0 = pool.make(res0, &goal_expr0);

    pool.pin(goal0);
    pool.trim();

    EXPECT_EQ(pool.make(res0, &goal_expr0),        goal0); // goal0 survived
    EXPECT_EQ(pool.make(nullptr, &rule_idx0), res0); // res0 (ancestor) survived
}

TEST_F(LineagePoolTest, PinLeafGoalPinsEntireChainToRoot) {
    const resolution_lineage* res0  = pool.make(nullptr, &rule_idx0);
    const goal_lineage*       goal0 = pool.make(res0, &goal_expr0);
    const resolution_lineage* res1  = pool.make(goal0, &rule_idx0);
    const goal_lineage*       goal1 = pool.make(res1, &goal_expr0);

    pool.pin(goal1);
    pool.trim();

    EXPECT_EQ(pool.make(nullptr, &rule_idx0), res0);
    EXPECT_EQ(pool.make(res0, &goal_expr0),          goal0);
    EXPECT_EQ(pool.make(goal0, &rule_idx0),   res1);
    EXPECT_EQ(pool.make(res1, &goal_expr0),          goal1);
}

TEST_F(LineagePoolTest, PinAlreadyPinnedAncestorStopsTraversal) {
    const resolution_lineage* res0  = pool.make(nullptr, &rule_idx0);
    const goal_lineage*       goal0 = pool.make(res0, &goal_expr0);
    const resolution_lineage* res1  = pool.make(goal0, &rule_idx0);
    const goal_lineage*       goal1 = pool.make(res1, &goal_expr0);

    pool.pin(goal0);  // pins res0 + goal0
    pool.pin(goal1);  // pins res1 + goal1; short-circuits at res0 (already pinned)
    pool.trim();

    EXPECT_EQ(pool.make(nullptr, &rule_idx0), res0);
    EXPECT_EQ(pool.make(res0, &goal_expr0),          goal0);
    EXPECT_EQ(pool.make(goal0, &rule_idx0),   res1);
    EXPECT_EQ(pool.make(res1, &goal_expr0),          goal1);
}

// ---------------------------------------------------------------------------
// trim removes unpinned, keeps pinned
// ---------------------------------------------------------------------------

TEST_F(LineagePoolTest, TrimKeepsPinned) {
    const goal_lineage* g0 = pool.make(nullptr, &goal_expr0);
    pool.make(nullptr, &goal_expr1);

    pool.pin(g0);
    pool.trim();

    EXPECT_EQ(pool.make(nullptr, &goal_expr0), g0);  // pinned: same pointer
}

TEST_F(LineagePoolTest, TrimRemovesUnpinnedResolutionUnderPinnedSiblingGoal) {
    const goal_lineage* g0 = pool.make(nullptr, &goal_expr0);
    const goal_lineage* g1 = pool.make(nullptr, &goal_expr1);
    const resolution_lineage* r1 = pool.make(g1, &rule_idx1);

    pool.pin(g0);
    pool.trim();

    const goal_lineage* g1_new = pool.make(nullptr, &goal_expr1);
    const resolution_lineage* r1_new = pool.make(g1_new, &rule_idx1);

    EXPECT_EQ(pool.make(nullptr, &goal_expr0), g0);
    EXPECT_NE(r1_new, r1);
}

TEST_F(LineagePoolTest, TrimIsIdempotentForPinnedItems) {
    const goal_lineage* g = pool.make(nullptr, &goal_expr0);
    pool.pin(g);
    pool.trim();
    pool.trim();
    EXPECT_EQ(pool.make(nullptr, &goal_expr0), g);
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
    resolution_lineage raw_res{nullptr, &rule_idx0};
    goal_lineage       raw_goal{&raw_res, &goal_expr0};

    const goal_lineage* p = pool.import(&raw_goal);
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(pool.import(&raw_goal), p); // idempotent
}

TEST_F(LineagePoolTest, ImportResolutionRecursivelyImportsParent) {
    goal_lineage       raw_parent{nullptr, &goal_expr0};
    resolution_lineage raw_leaf{&raw_parent, &rule_idx0};

    const resolution_lineage* leaf = pool.import(&raw_leaf);
    EXPECT_NE(leaf, nullptr);

    const goal_lineage* parent = pool.make(nullptr, &goal_expr0);
    EXPECT_EQ(leaf->parent, parent); // parent was also interned into the pool
    EXPECT_EQ(pool.import(&raw_parent), parent); // re-import of root returns same ptr
}
