#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_driver.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockPropagateUnit {
    MOCK_METHOD(pud_unfold_outcome, propagate, ());
};
struct MockNextChoice {
    MOCK_METHOD(std::optional<pud_lodestar_choice>, next, ());
};
struct MockUnfoldLeaf {
    MOCK_METHOD(pud_unfold_outcome, unfold, (pud_global_node_id, pud_body_goal_id));
};

using test_driver_t = pud_lodestar_driver<
    NiceMock<MockPropagateUnit>, NiceMock<MockNextChoice>, NiceMock<MockNextChoice>, NiceMock<MockUnfoldLeaf>>;

pud_lodestar_choice choice_on(pud_global_node_id leaf, pud_body_goal_id goal) {
    return pud_lodestar_choice{
        .slot = pud_lodestar_slot_id{
            .query_node = leaf, .query_goal = goal, .candidate = 0, .side = pud_lodestar_side::self},
        .leaf = leaf,
        .body_goal = goal,
    };
}

// The outer ring deals a choice on leaf 7 goal 1, the conjecture ring one on leaf 8 goal 2.
struct PudLodestarDriverTest : public ::testing::Test {
    NiceMock<MockPropagateUnit> units;
    NiceMock<MockNextChoice> next_outer;
    NiceMock<MockNextChoice> next_inner;
    NiceMock<MockUnfoldLeaf> unfolder;
    pud_lodestar_choice outer = choice_on(pud_test_node(7), 1);
    pud_lodestar_choice inner = choice_on(pud_test_node(8), 2);

    void SetUp() override {
        ON_CALL(units, propagate()).WillByDefault(Return(pud_unfold_outcome::none));
        ON_CALL(next_outer, next()).WillByDefault(Return(outer));
        ON_CALL(next_inner, next()).WillByDefault(Return(inner));
        ON_CALL(unfolder, unfold(pud_test_node(7), 1)).WillByDefault(Return(pud_unfold_outcome::off_focus));
        ON_CALL(unfolder, unfold(pud_test_node(8), 2)).WillByDefault(Return(pud_unfold_outcome::on_focus));
    }

    test_driver_t make_driver(size_t ratio) {
        return test_driver_t{units, next_outer, next_inner, unfolder, ratio};
    }
};

}

TEST_F(PudLodestarDriverTest, AUnitUnfoldComesFirstAndDealsNoChoice) {
    test_driver_t driver = make_driver(1);
    EXPECT_CALL(units, propagate()).WillOnce(Return(pud_unfold_outcome::on_focus));
    EXPECT_CALL(next_outer, next()).Times(0);
    EXPECT_CALL(next_inner, next()).Times(0);
    EXPECT_EQ(driver.unfold_one(), pud_unfold_outcome::on_focus);
}

TEST_F(PudLodestarDriverTest, OnTheOuterTurnTheOuterRingIsDealtFirst) {
    test_driver_t driver = make_driver(1);
    EXPECT_CALL(next_inner, next()).Times(0);
    EXPECT_CALL(unfolder, unfold(pud_test_node(7), 1));
    EXPECT_EQ(driver.unfold_one(), pud_unfold_outcome::off_focus);
}

TEST_F(PudLodestarDriverTest, ARatioOfZeroDealsTheConjectureRingFirst) {
    test_driver_t driver = make_driver(0);
    EXPECT_CALL(next_outer, next()).Times(0);
    EXPECT_CALL(unfolder, unfold(pud_test_node(8), 2));
    EXPECT_EQ(driver.unfold_one(), pud_unfold_outcome::on_focus);
}

TEST_F(PudLodestarDriverTest, AnEmptyOuterRingOnItsTurnFallsThroughToTheConjectureRing) {
    test_driver_t driver = make_driver(1);
    ON_CALL(next_outer, next()).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(unfolder, unfold(pud_test_node(8), 2));
    EXPECT_EQ(driver.unfold_one(), pud_unfold_outcome::on_focus);
}

TEST_F(PudLodestarDriverTest, AnEmptyConjectureRingOffTheOuterTurnFallsBackToTheOuterRing) {
    test_driver_t driver = make_driver(0);
    ON_CALL(next_inner, next()).WillByDefault(Return(std::nullopt));
    InSequence order;
    EXPECT_CALL(next_inner, next());
    EXPECT_CALL(next_outer, next());
    EXPECT_CALL(unfolder, unfold(pud_test_node(7), 1));
    EXPECT_EQ(driver.unfold_one(), pud_unfold_outcome::off_focus);
}

TEST_F(PudLodestarDriverTest, BothRingsEmptyMakesNoUnfoldAndAsksTheOuterRingOnce) {
    test_driver_t driver = make_driver(1);
    EXPECT_CALL(next_outer, next()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(next_inner, next()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(unfolder, unfold(_, _)).Times(0);
    EXPECT_EQ(driver.unfold_one(), pud_unfold_outcome::none);
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
        .WillOnce(Return(pud_unfold_outcome::off_focus))
        .WillOnce(Return(pud_unfold_outcome::none));
    EXPECT_CALL(unfolder, unfold(pud_test_node(8), 2));
    driver.unfold_one();
    EXPECT_EQ(driver.unfold_one(), pud_unfold_outcome::on_focus);
}
