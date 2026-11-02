#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_driver.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockPropagateUnit {
    MOCK_METHOD(pud_lodestar_unfold_outcome, propagate, ());
};
struct MockListQueries {
    MOCK_METHOD(std::vector<pud_lodestar_query>, list, (), (const));
};
struct MockBuildRing {
    MOCK_METHOD(std::vector<pud_lodestar_choice>, build, ((const std::vector<pud_lodestar_query>&), bool), (const));
};
struct MockNextChoice {
    MOCK_METHOD(std::optional<pud_lodestar_choice>, next, ((const std::vector<pud_lodestar_choice>&)));
};
struct MockUnfoldLeaf {
    MOCK_METHOD(pud_lodestar_unfold_outcome, unfold, (pud_global_node_id, pud_body_goal_id));
};

using test_driver_t = pud_lodestar_driver<
    NiceMock<MockPropagateUnit>, NiceMock<MockListQueries>, NiceMock<MockBuildRing>, NiceMock<MockNextChoice>,
    NiceMock<MockNextChoice>, NiceMock<MockUnfoldLeaf>>;

pud_lodestar_choice choice_on(pud_global_node_id leaf, pud_body_goal_id goal) {
    return pud_lodestar_choice{
        .slot = pud_lodestar_slot_id{
            .query_node = leaf, .query_goal = goal, .candidate = 0, .side = pud_lodestar_side::self},
        .leaf = leaf,
        .body_goal = goal,
    };
}

// The outer ring holds one choice on leaf 7 goal 1, the conjecture ring one on leaf 8 goal 2.
struct PudLodestarDriverTest : public ::testing::Test {
    NiceMock<MockPropagateUnit> units;
    NiceMock<MockListQueries> queries;
    NiceMock<MockBuildRing> rings;
    NiceMock<MockNextChoice> next_outer;
    NiceMock<MockNextChoice> next_inner;
    NiceMock<MockUnfoldLeaf> unfolder;
    std::vector<pud_lodestar_query> listed{
        pud_lodestar_query{.node = pud_test_node(3), .goal = 0, .head = 9, .has_candidates = true}};
    pud_lodestar_choice outer = choice_on(pud_test_node(7), 1);
    pud_lodestar_choice inner = choice_on(pud_test_node(8), 2);
    std::vector<pud_lodestar_choice> outer_ring{outer};
    std::vector<pud_lodestar_choice> inner_ring{inner};

    void SetUp() override {
        ON_CALL(units, propagate()).WillByDefault(Return(pud_lodestar_unfold_outcome::none));
        ON_CALL(queries, list()).WillByDefault(Return(listed));
        ON_CALL(rings, build(listed, false)).WillByDefault(Return(outer_ring));
        ON_CALL(rings, build(listed, true)).WillByDefault(Return(inner_ring));
        ON_CALL(next_outer, next(outer_ring)).WillByDefault(Return(outer));
        ON_CALL(next_inner, next(inner_ring)).WillByDefault(Return(inner));
        ON_CALL(unfolder, unfold(pud_test_node(7), 1)).WillByDefault(Return(pud_lodestar_unfold_outcome::off_focus));
        ON_CALL(unfolder, unfold(pud_test_node(8), 2)).WillByDefault(Return(pud_lodestar_unfold_outcome::on_focus));
    }

    test_driver_t make_driver(size_t ratio) {
        return test_driver_t{units, queries, rings, next_outer, next_inner, unfolder, ratio};
    }
};

}

TEST_F(PudLodestarDriverTest, AUnitUnfoldComesFirstAndBuildsNoRing) {
    test_driver_t driver = make_driver(1);
    EXPECT_CALL(units, propagate()).WillOnce(Return(pud_lodestar_unfold_outcome::on_focus));
    EXPECT_CALL(queries, list()).Times(0);
    EXPECT_CALL(rings, build(_, _)).Times(0);
    EXPECT_EQ(driver.unfold_one(), pud_lodestar_unfold_outcome::on_focus);
}

TEST_F(PudLodestarDriverTest, OnTheOuterTurnTheOuterRingIsDealtFirst) {
    test_driver_t driver = make_driver(1);
    EXPECT_CALL(rings, build(_, _)).Times(AnyNumber());
    EXPECT_CALL(rings, build(listed, true)).Times(0);
    EXPECT_CALL(unfolder, unfold(pud_test_node(7), 1));
    EXPECT_EQ(driver.unfold_one(), pud_lodestar_unfold_outcome::off_focus);
}

TEST_F(PudLodestarDriverTest, ARatioOfZeroDealsTheConjectureRingFirst) {
    test_driver_t driver = make_driver(0);
    EXPECT_CALL(rings, build(_, _)).Times(AnyNumber());
    EXPECT_CALL(rings, build(listed, false)).Times(0);
    EXPECT_CALL(unfolder, unfold(pud_test_node(8), 2));
    EXPECT_EQ(driver.unfold_one(), pud_lodestar_unfold_outcome::on_focus);
}

TEST_F(PudLodestarDriverTest, AnEmptyOuterRingOnItsTurnFallsThroughToTheConjectureRing) {
    test_driver_t driver = make_driver(1);
    ON_CALL(next_outer, next(_)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(unfolder, unfold(pud_test_node(8), 2));
    EXPECT_EQ(driver.unfold_one(), pud_lodestar_unfold_outcome::on_focus);
}

TEST_F(PudLodestarDriverTest, AnEmptyConjectureRingOffTheOuterTurnFallsBackToTheOuterRing) {
    test_driver_t driver = make_driver(0);
    ON_CALL(next_inner, next(_)).WillByDefault(Return(std::nullopt));
    InSequence order;
    EXPECT_CALL(rings, build(listed, true));
    EXPECT_CALL(rings, build(listed, false));
    EXPECT_CALL(unfolder, unfold(pud_test_node(7), 1));
    EXPECT_EQ(driver.unfold_one(), pud_lodestar_unfold_outcome::off_focus);
}

TEST_F(PudLodestarDriverTest, BothRingsEmptyMakesNoUnfoldAndBuildsTheOuterRingOnce) {
    test_driver_t driver = make_driver(1);
    ON_CALL(next_outer, next(_)).WillByDefault(Return(std::nullopt));
    ON_CALL(next_inner, next(_)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(rings, build(_, _)).Times(AnyNumber());
    EXPECT_CALL(rings, build(listed, false)).Times(1);
    EXPECT_CALL(unfolder, unfold(_, _)).Times(0);
    EXPECT_EQ(driver.unfold_one(), pud_lodestar_unfold_outcome::none);
}

TEST_F(PudLodestarDriverTest, OuterUnfoldsUseUpTheRatioAndAConjectureUnfoldRestoresIt) {
    test_driver_t driver = make_driver(2);
    InSequence order;
    EXPECT_CALL(unfolder, unfold(pud_test_node(7), 1));
    EXPECT_CALL(unfolder, unfold(pud_test_node(7), 1));
    EXPECT_CALL(unfolder, unfold(pud_test_node(8), 2));
    EXPECT_CALL(unfolder, unfold(pud_test_node(7), 1));
    driver.unfold_one();
    driver.unfold_one();
    driver.unfold_one();
    driver.unfold_one();
}

TEST_F(PudLodestarDriverTest, AnOffFocusUnitUnfoldCountsAgainstTheRatio) {
    test_driver_t driver = make_driver(1);
    EXPECT_CALL(units, propagate())
        .WillOnce(Return(pud_lodestar_unfold_outcome::off_focus))
        .WillOnce(Return(pud_lodestar_unfold_outcome::none));
    EXPECT_CALL(unfolder, unfold(pud_test_node(8), 2));
    driver.unfold_one();
    EXPECT_EQ(driver.unfold_one(), pud_lodestar_unfold_outcome::on_focus);
}
