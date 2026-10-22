#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/ubs_bind_map.hpp"
#include "ubs_bind_map_test_maps.hpp"

namespace {

using test_ubs_bind_map_t = ubs_bind_map<
    ubs_std_parent_map, ubs_std_parent_map, ubs_std_size_map, ubs_std_size_map,
    ubs_std_rep_map, ubs_std_rep_map, ubs_std_value_map, ubs_std_value_map>;

struct UbsBindMapTest : public ::testing::Test {
    ubs_std_parent_map parents;
    ubs_std_size_map sizes;
    ubs_std_rep_map reps;
    ubs_std_value_map values;
    test_ubs_bind_map_t store{parents, parents, sizes, sizes, reps, reps, values, values};
    expr leaf{expr::functor{7, {}}};
    expr other_leaf{expr::functor{8, {}}};

    ubs_map_snapshot snapshot() const {
        return snapshot_ubs_maps(parents, sizes, reps, values);
    }
};

}

TEST_F(UbsBindMapTest, FreshVarIsItsOwnUnboundRepresentative) {
    EXPECT_EQ(store.find(5), 5u);
    EXPECT_EQ(store.find_rep(5), 5u);
    EXPECT_FALSE(store.find_value(5).has_value());
}

TEST_F(UbsBindMapTest, ReadsDoNotMutateMaps) {
    store.equate(3, 4);
    store.instantiate(3, framed_expr{&leaf, 0});
    const ubs_map_snapshot before = snapshot();
    store.find(4);
    store.find_rep(4);
    store.find_value(4);
    store.find(99);
    EXPECT_TRUE(ubs_snapshots_equal(before, snapshot()));
}

TEST_F(UbsBindMapTest, EquateGivesTheClassItsLeastVar) {
    store.equate(9, 2);
    EXPECT_EQ(store.find_rep(9), 2u);
    EXPECT_EQ(store.find_rep(2), 2u);
    EXPECT_EQ(store.find(9), store.find(2));
}

TEST_F(UbsBindMapTest, EquateArgumentOrderIsIrrelevant) {
    ubs_std_parent_map other_parents;
    ubs_std_size_map other_sizes;
    ubs_std_rep_map other_reps;
    ubs_std_value_map other_values;
    test_ubs_bind_map_t other{
        other_parents, other_parents, other_sizes, other_sizes, other_reps, other_reps, other_values, other_values};
    store.equate(1, 2);
    other.equate(2, 1);
    EXPECT_EQ(store.find_rep(1), other.find_rep(1));
    EXPECT_EQ(store.find_rep(2), other.find_rep(2));
}

TEST_F(UbsBindMapTest, EquateSameClassIsAPreconditionViolation) {
    store.equate(1, 2);
    EXPECT_THROW(store.equate(2, 1), std::logic_error);
    EXPECT_THROW(store.equate(4, 4), std::logic_error);
}

TEST_F(UbsBindMapTest, EquateLargerClassRootSurvives) {
    store.equate(10, 11);
    store.equate(10, 12);
    const uint32_t root = store.find(10);
    store.equate(20, 10);
    EXPECT_EQ(store.find(20), root);
    EXPECT_EQ(*sizes.get_size(root), 4u);
}

TEST_F(UbsBindMapTest, EquateWritesOneParentAndNoneForTheSurvivor) {
    store.equate(1, 2);
    EXPECT_EQ(parents.map.size(), 1u);
    EXPECT_EQ(parents.get_parent(store.find(1)), nullptr);
}

TEST_F(UbsBindMapTest, EquateKeepsTheRepresentativeTheLeastOfBothClasses) {
    store.equate(30, 31);
    store.equate(30, 32);
    store.equate(5, 6);
    store.equate(31, 6);
    for (uint32_t member : {5u, 6u, 30u, 31u, 32u})
        EXPECT_EQ(store.find_rep(member), 5u) << member;
}

TEST_F(UbsBindMapTest, EquateBoundWithUnboundKeepsTheValue) {
    store.instantiate(4, framed_expr{&leaf, 3});
    store.equate(1, 4);
    EXPECT_EQ(store.find_value(1), (framed_expr{&leaf, 3}));
    EXPECT_EQ(store.find_rep(4), 1u);
}

TEST_F(UbsBindMapTest, EquateUnboundIntoSmallerBoundClassMovesTheValueToTheSurvivor) {
    store.equate(1, 2);
    store.equate(1, 3);
    store.instantiate(9, framed_expr{&leaf, 0});
    store.equate(1, 9);
    EXPECT_EQ(store.find_value(9), (framed_expr{&leaf, 0}));
    EXPECT_EQ(store.find_value(2), (framed_expr{&leaf, 0}));
}

TEST_F(UbsBindMapTest, EquateTwoBoundClassesKeepsOneValue) {
    store.instantiate(1, framed_expr{&leaf, 0});
    store.instantiate(2, framed_expr{&other_leaf, 0});
    store.equate(1, 2);
    const std::optional<framed_expr> value = store.find_value(2);
    ASSERT_TRUE(value.has_value());
    EXPECT_TRUE(value == (framed_expr{&leaf, 0}) || value == (framed_expr{&other_leaf, 0}));
    EXPECT_EQ(store.find_value(1), value);
}

TEST_F(UbsBindMapTest, InstantiateBindsTheWholeClassAtItsRoot) {
    store.equate(1, 2);
    store.equate(1, 3);
    store.instantiate(3, framed_expr{&leaf, 5});
    EXPECT_EQ(values.map.size(), 1u);
    EXPECT_NE(values.get_value(store.find(1)), nullptr);
    for (uint32_t member : {1u, 2u, 3u})
        EXPECT_EQ(store.find_value(member), (framed_expr{&leaf, 5})) << member;
}

TEST_F(UbsBindMapTest, InstantiateKeepsTheRepresentative) {
    store.equate(1, 2);
    store.instantiate(2, framed_expr{&leaf, 0});
    EXPECT_EQ(store.find_rep(2), 1u);
}

TEST_F(UbsBindMapTest, InstantiateSingletonWritesOnlyItsValue) {
    store.instantiate(6, framed_expr{&leaf, 0});
    EXPECT_TRUE(parents.map.empty());
    EXPECT_TRUE(sizes.map.empty());
    EXPECT_TRUE(reps.map.empty());
}

TEST_F(UbsBindMapTest, InstantiateBoundClassIsAPreconditionViolation) {
    store.equate(1, 2);
    store.instantiate(1, framed_expr{&leaf, 0});
    EXPECT_THROW(store.instantiate(2, framed_expr{&other_leaf, 0}), std::logic_error);
}

TEST_F(UbsBindMapTest, StaleNonRootEntriesAreNeverRead) {
    store.instantiate(4, framed_expr{&leaf, 0});
    store.equate(1, 2);
    store.equate(1, 3);
    store.equate(1, 4);
    EXPECT_EQ(store.find_value(4), (framed_expr{&leaf, 0}));
    EXPECT_EQ(store.find_rep(4), 1u);
}

TEST_F(UbsBindMapTest, UnionBySizeHeightIsLogarithmic) {
    std::mt19937 rng{3};
    constexpr uint32_t var_count = 1024;
    std::vector<uint32_t> order(var_count);
    for (uint32_t var = 0; var < var_count; ++var)
        order[var] = var;
    std::shuffle(order.begin(), order.end(), rng);
    for (uint32_t index = 1; index < var_count; ++index)
        store.equate(order[index - 1], order[index]);
    uint32_t deepest = 0;
    for (uint32_t var = 0; var < var_count; ++var)
        deepest = std::max(deepest, ubs_parent_chain_depth(parents, var));
    EXPECT_LE(deepest, static_cast<uint32_t>(std::log2(var_count)));
    EXPECT_EQ(store.find_rep(order.back()), 0u);
}
