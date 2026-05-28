// lineage_pool interns goal and resolution lineages, pins ancestry for retention, and
// trims unpinned nodes. Unit tests cover intern identity, pin propagation, trim GC,
// and import without external mocks (no i_* dependencies).

#include <gtest/gtest.h>
#include "infrastructure/lineage_pool.hpp"
struct LineagePoolTest : public ::testing::Test {
protected:
    lineage_pool pool;

    const goal_lineage* make_root_goal(subgoal_id idx) {
        return pool.make_goal_lineage(nullptr, idx);
    }

    const resolution_lineage* make_root_resolution(rule_id idx) {
        return pool.make_resolution_lineage(nullptr, idx);
    }
};

// ---------------------------------------------------------------------------
// Interning / identity
// ---------------------------------------------------------------------------

TEST_F(LineagePoolTest, GoalLineageInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(make_root_goal(0), make_root_goal(0));
}

TEST_F(LineagePoolTest, DifferentGoalIndicesReturnDifferentPointers) {
    EXPECT_NE(make_root_goal(0), make_root_goal(1));
}

TEST_F(LineagePoolTest, ResolutionLineageInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(make_root_resolution(0), make_root_resolution(0));
}

TEST_F(LineagePoolTest, DifferentResolutionIndicesReturnDifferentPointers) {
    EXPECT_NE(make_root_resolution(0), make_root_resolution(1));
}

TEST_F(LineagePoolTest, GoalWithParentInternedTwiceReturnsSamePointer) {
    const resolution_lineage* res = make_root_resolution(0);
    EXPECT_EQ(pool.make_goal_lineage(res, subgoal_id{0}), pool.make_goal_lineage(res, subgoal_id{0}));
}

TEST_F(LineagePoolTest, GoalWithDifferentParentsReturnDifferentPointers) {
    const resolution_lineage* res0 = make_root_resolution(0);
    const resolution_lineage* res1 = make_root_resolution(1);
    EXPECT_NE(pool.make_goal_lineage(res0, subgoal_id{0}), pool.make_goal_lineage(res1, subgoal_id{0}));
}

// ---------------------------------------------------------------------------
// pin propagates up the ancestry chain
//
// Chain used in all pin tests:
//   res0 (nullptr parent)
//     └── goal0 (parent = res0, subgoal 0)
//           └── res1 (parent = goal0, rule 0)
//                 └── goal1 (parent = res1, subgoal 0)
// ---------------------------------------------------------------------------

TEST_F(LineagePoolTest, PinGoalPinsItsResolutionParent) {
    const resolution_lineage* res0  = make_root_resolution(0);
    const goal_lineage*       goal0 = pool.make_goal_lineage(res0, subgoal_id{0});

    pool.pin(goal0);
    pool.trim();

    EXPECT_EQ(pool.make_goal_lineage(res0, subgoal_id{0}), goal0);
    EXPECT_EQ(make_root_resolution(0), res0);
}

TEST_F(LineagePoolTest, PinLeafGoalPinsEntireChainToRoot) {
    const resolution_lineage* res0  = make_root_resolution(0);
    const goal_lineage*       goal0 = pool.make_goal_lineage(res0, subgoal_id{0});
    const resolution_lineage* res1  = pool.make_resolution_lineage(goal0, rule_id{0});
    const goal_lineage*       goal1 = pool.make_goal_lineage(res1, subgoal_id{0});

    pool.pin(goal1);
    pool.trim();

    EXPECT_EQ(make_root_resolution(0), res0);
    EXPECT_EQ(pool.make_goal_lineage(res0, subgoal_id{0}), goal0);
    EXPECT_EQ(pool.make_resolution_lineage(goal0, rule_id{0}), res1);
    EXPECT_EQ(pool.make_goal_lineage(res1, subgoal_id{0}), goal1);
}

TEST_F(LineagePoolTest, PinAlreadyPinnedAncestorStopsTraversal) {
    const resolution_lineage* res0  = make_root_resolution(0);
    const goal_lineage*       goal0 = pool.make_goal_lineage(res0, subgoal_id{0});
    const resolution_lineage* res1  = pool.make_resolution_lineage(goal0, rule_id{0});
    const goal_lineage*       goal1 = pool.make_goal_lineage(res1, subgoal_id{0});

    pool.pin(goal0);
    pool.pin(goal1);
    pool.trim();

    EXPECT_EQ(make_root_resolution(0), res0);
    EXPECT_EQ(pool.make_goal_lineage(res0, subgoal_id{0}), goal0);
    EXPECT_EQ(pool.make_resolution_lineage(goal0, rule_id{0}), res1);
    EXPECT_EQ(pool.make_goal_lineage(res1, subgoal_id{0}), goal1);
}

// ---------------------------------------------------------------------------
// trim removes unpinned, keeps pinned
// ---------------------------------------------------------------------------

TEST_F(LineagePoolTest, TrimKeepsPinned) {
    const goal_lineage* g0 = make_root_goal(0);
    make_root_goal(1);

    pool.pin(g0);
    pool.trim();

    EXPECT_EQ(make_root_goal(0), g0);
}

TEST_F(LineagePoolTest, TrimRemovesUnpinnedResolutionUnderPinnedSiblingGoal) {
    const goal_lineage* g0 = make_root_goal(0);
    const goal_lineage* g1 = make_root_goal(1);
    const resolution_lineage* r1 = pool.make_resolution_lineage(g1, rule_id{1});

    pool.pin(g0);
    pool.trim();

    const goal_lineage* g1_new = make_root_goal(1);
    const resolution_lineage* r1_new = pool.make_resolution_lineage(g1_new, rule_id{1});

    EXPECT_EQ(make_root_goal(0), g0);
    EXPECT_NE(r1_new, r1);
}

TEST_F(LineagePoolTest, TrimIsIdempotentForPinnedItems) {
    const goal_lineage* g = make_root_goal(0);
    pool.pin(g);
    pool.trim();
    pool.trim();
    EXPECT_EQ(make_root_goal(0), g);
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
    resolution_lineage raw_res{nullptr, rule_id{0}};
    goal_lineage       raw_goal{&raw_res, subgoal_id{0}};

    const goal_lineage* p = pool.import(&raw_goal);
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(pool.import(&raw_goal), p);
}

TEST_F(LineagePoolTest, ImportResolutionRecursivelyImportsParent) {
    goal_lineage       raw_parent{nullptr, subgoal_id{0}};
    resolution_lineage raw_leaf{&raw_parent, rule_id{0}};

    const resolution_lineage* leaf = pool.import(&raw_leaf);
    EXPECT_NE(leaf, nullptr);

    const goal_lineage* parent = make_root_goal(0);
    EXPECT_EQ(leaf->parent, parent);
    EXPECT_EQ(pool.import(&raw_parent), parent);
}
