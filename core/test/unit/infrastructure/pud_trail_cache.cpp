#include <gtest/gtest.h>
#include "infrastructure/pud_trail_cache.hpp"
#include "pud_test_node.hpp"
#include "value_objects/pud_goal_trails.hpp"
#include "value_objects/pud_node_trails.hpp"

namespace {

pud_node_trails trails_for_goal(pud_body_goal_id goal) {
    return pud_node_trails{}.set(goal, pud_goal_trails{.candidates = {}, .forked = false});
}

struct PudTrailCacheTest : public ::testing::Test {
    pud_trail_cache cache{1 << 20};
};

}

TEST_F(PudTrailCacheTest, ANodeWithoutStoredTrailsHasNone) {
    EXPECT_EQ(cache.find(pud_test_node(1)), nullptr);
}

TEST_F(PudTrailCacheTest, StoredTrailsAreFoundAndReplacedByTheNextStore) {
    cache.store(pud_test_node(1), trails_for_goal(3));
    cache.store(pud_test_node(1), trails_for_goal(4));
    ASSERT_NE(cache.find(pud_test_node(1)), nullptr);
    EXPECT_EQ(*cache.find(pud_test_node(1)), trails_for_goal(4));
}

TEST_F(PudTrailCacheTest, ErasingANodeWithoutTrailsDoesNothing) {
    cache.erase(pud_test_node(1));
    cache.store(pud_test_node(2), trails_for_goal(3));
    cache.erase(pud_test_node(2));
    EXPECT_EQ(cache.find(pud_test_node(2)), nullptr);
}

TEST(PudTrailCacheBudgetTest, TrimmingDropsTheLeastRecentlyUsedTrailsUntilTheBudgetHolds) {
    pud_trail_cache cache{0};
    cache.store(pud_test_node(1), trails_for_goal(3));
    cache.store(pud_test_node(2), trails_for_goal(3));
    EXPECT_NE(cache.find(pud_test_node(1)), nullptr);
    cache.trim();
    EXPECT_EQ(cache.find(pud_test_node(1)), nullptr);
    EXPECT_EQ(cache.find(pud_test_node(2)), nullptr);
}
