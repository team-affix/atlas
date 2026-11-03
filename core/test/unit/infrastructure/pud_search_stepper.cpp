#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_search_stepper.hpp"
#include "value_objects/pud_leaf_goal_query.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockClearSolution {
    MOCK_METHOD(void, clear, ());
};
struct MockTakePendingConflict {
    MOCK_METHOD(bool, try_take, ());
};
struct MockTakeSolution {
    MOCK_METHOD(bool, take, ());
};
struct MockRefuteBlocked {
    MOCK_METHOD(bool, refute_blocked, ());
};
struct MockUnfoldOne {
    MOCK_METHOD(pud_unfold_outcome, unfold_one, ());
};
struct MockListQueries {
    MOCK_METHOD(std::vector<pud_leaf_goal_query>, list, (), (const));
};

using test_stepper_t = pud_search_stepper<
    NiceMock<MockClearSolution>, NiceMock<MockTakePendingConflict>, NiceMock<MockTakeSolution>,
    NiceMock<MockRefuteBlocked>, NiceMock<MockUnfoldOne>, NiceMock<MockListQueries>>;

// By default nothing is pending, solved or blocked, and every unfold succeeds off the focus.
struct PudSearchStepperTest : public ::testing::Test {
    NiceMock<MockClearSolution> solutions;
    NiceMock<MockTakePendingConflict> conflicts;
    NiceMock<MockTakeSolution> taker;
    NiceMock<MockRefuteBlocked> refuter;
    NiceMock<MockUnfoldOne> driver;
    NiceMock<MockListQueries> queries;
    std::vector<pud_leaf_goal_query> open_query{
        pud_leaf_goal_query{.node = pud_test_node(3), .goal = 0, .head = 40, .has_candidates = true}};

    void SetUp() override {
        ON_CALL(conflicts, try_take()).WillByDefault(Return(false));
        ON_CALL(taker, take()).WillByDefault(Return(false));
        ON_CALL(refuter, refute_blocked()).WillByDefault(Return(false));
        ON_CALL(driver, unfold_one()).WillByDefault(Return(pud_unfold_outcome::off_focus));
        ON_CALL(queries, list()).WillByDefault(Return(std::vector<pud_leaf_goal_query>{}));
    }

    test_stepper_t make_stepper(size_t max_unfolds) {
        return test_stepper_t{solutions, conflicts, taker, refuter, driver, queries, max_unfolds};
    }
};

}

TEST_F(PudSearchStepperTest, ANewSessionIsRunning) {
    const test_stepper_t stepper = make_stepper(4);
    EXPECT_EQ(stepper.stop_reason(), pud_search_stop_reason::running);
}

TEST_F(PudSearchStepperTest, EveryTickFirstClearsTheLastSolution) {
    test_stepper_t stepper = make_stepper(4);
    ON_CALL(taker, take()).WillByDefault(Return(true));
    InSequence order;
    EXPECT_CALL(solutions, clear());
    EXPECT_CALL(taker, take());
    stepper.step();
}

TEST_F(PudSearchStepperTest, APendingConflictIsATickWithoutSearching) {
    test_stepper_t stepper = make_stepper(4);
    ON_CALL(conflicts, try_take()).WillByDefault(Return(true));
    EXPECT_CALL(taker, take()).Times(0);
    EXPECT_TRUE(stepper.step());
}

TEST_F(PudSearchStepperTest, ATakenSolutionEndsTheTickBeforeAnyUnfold) {
    test_stepper_t stepper = make_stepper(4);
    ON_CALL(taker, take()).WillByDefault(Return(true));
    EXPECT_CALL(refuter, refute_blocked()).Times(0);
    EXPECT_CALL(driver, unfold_one()).Times(0);
    EXPECT_TRUE(stepper.step());
}

TEST_F(PudSearchStepperTest, ABlockedRefutationEndsTheTickBeforeAnyUnfold) {
    test_stepper_t stepper = make_stepper(4);
    ON_CALL(refuter, refute_blocked()).WillByDefault(Return(true));
    EXPECT_CALL(driver, unfold_one()).Times(0);
    EXPECT_TRUE(stepper.step());
}

TEST_F(PudSearchStepperTest, UnfoldsContinueUntilASolutionIsTaken) {
    test_stepper_t stepper = make_stepper(4);
    EXPECT_CALL(taker, take()).WillOnce(Return(false)).WillOnce(Return(false)).WillOnce(Return(true));
    EXPECT_CALL(driver, unfold_one()).Times(2);
    EXPECT_TRUE(stepper.step());
}

TEST_F(PudSearchStepperTest, EveryUnfoldOnOrOffTheFocusSpendsTheBudget) {
    test_stepper_t stepper = make_stepper(2);
    EXPECT_CALL(driver, unfold_one())
        .WillOnce(Return(pud_unfold_outcome::off_focus))
        .WillOnce(Return(pud_unfold_outcome::on_focus));
    EXPECT_FALSE(stepper.step());
    EXPECT_EQ(stepper.stop_reason(), pud_search_stop_reason::budget_spent);
}

TEST_F(PudSearchStepperTest, TheBudgetCarriesAcrossTicks) {
    test_stepper_t stepper = make_stepper(2);
    EXPECT_CALL(taker, take()).WillOnce(Return(false)).WillOnce(Return(true)).WillRepeatedly(Return(false));
    EXPECT_CALL(driver, unfold_one()).Times(2);
    EXPECT_TRUE(stepper.step());
    EXPECT_FALSE(stepper.step());
    EXPECT_EQ(stepper.stop_reason(), pud_search_stop_reason::budget_spent);
}

TEST_F(PudSearchStepperTest, ABudgetOfZeroUnfoldsNothing) {
    test_stepper_t stepper = make_stepper(0);
    EXPECT_CALL(driver, unfold_one()).Times(0);
    EXPECT_FALSE(stepper.step());
    EXPECT_EQ(stepper.stop_reason(), pud_search_stop_reason::budget_spent);
}

TEST_F(PudSearchStepperTest, NoUnfoldWithNoOpenConjectureLeafIsARefutation) {
    test_stepper_t stepper = make_stepper(4);
    ON_CALL(driver, unfold_one()).WillByDefault(Return(pud_unfold_outcome::none));
    EXPECT_FALSE(stepper.step());
    EXPECT_EQ(stepper.stop_reason(), pud_search_stop_reason::refuted);
}

TEST_F(PudSearchStepperTest, NoUnfoldWhileAConjectureLeafIsStillOpenIsExhaustion) {
    test_stepper_t stepper = make_stepper(4);
    ON_CALL(driver, unfold_one()).WillByDefault(Return(pud_unfold_outcome::none));
    ON_CALL(queries, list()).WillByDefault(Return(open_query));
    EXPECT_FALSE(stepper.step());
    EXPECT_EQ(stepper.stop_reason(), pud_search_stop_reason::exhausted);
}

TEST_F(PudSearchStepperTest, AStoppedSessionStaysStoppedWithoutSearching) {
    test_stepper_t stepper = make_stepper(0);
    stepper.step();
    EXPECT_CALL(solutions, clear());
    EXPECT_CALL(conflicts, try_take()).Times(0);
    EXPECT_CALL(taker, take()).Times(0);
    EXPECT_FALSE(stepper.step());
    EXPECT_EQ(stepper.stop_reason(), pud_search_stop_reason::budget_spent);
}

TEST_F(PudSearchStepperTest, TheUnfoldCountIsEveryUnfoldMadeSoFar) {
    test_stepper_t stepper = make_stepper(4);
    EXPECT_EQ(stepper.unfold_count(), 0u);
    EXPECT_CALL(taker, take()).WillOnce(Return(false)).WillOnce(Return(false)).WillOnce(Return(true));
    stepper.step();
    EXPECT_EQ(stepper.unfold_count(), 2u);
}
