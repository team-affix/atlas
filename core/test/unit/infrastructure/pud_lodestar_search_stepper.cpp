#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_search_stepper.hpp"

using ::testing::InSequence;
using ::testing::Return;

namespace {

struct MockClearSolution {
    MOCK_METHOD(void, clear, ());
};

struct MockCountSolutions {
    MOCK_METHOD(size_t, solution_count, (), (const));
};

struct MockGetPendingConflicts {
    MOCK_METHOD(size_t, get, (), (const));
};

struct MockSetPendingConflicts {
    MOCK_METHOD(void, set, (size_t));
};

struct MockGetStopped {
    MOCK_METHOD(bool, get_stopped, (), (const));
};

struct MockSetStopped {
    MOCK_METHOD(void, set_stopped, (bool));
};

struct MockGetFocusUnfolds {
    MOCK_METHOD(size_t, get_focus_unfolds, (), (const));
};

struct MockSetFocusUnfolds {
    MOCK_METHOD(void, set_focus_unfolds, (size_t));
};

struct MockTakeSolution {
    MOCK_METHOD(bool, take, ());
};

struct MockRefuteBlocked {
    MOCK_METHOD(bool, refute_blocked, ());
};

struct MockUnfoldOne {
    MOCK_METHOD(bool, unfold_one, ());
};

struct MockCheckLastUnfoldOnFocus {
    MOCK_METHOD(bool, last_unfold_on_focus, (), (const));
};

struct PudLodestarSearchStepperTest : public ::testing::Test {
    MockClearSolution clear;
    MockCountSolutions counted;
    MockGetPendingConflicts pending;
    MockSetPendingConflicts set_pending;
    MockGetStopped stopped;
    MockSetStopped set_stopped;
    MockGetFocusUnfolds focus_unfolds;
    MockSetFocusUnfolds set_focus_unfolds;
    MockTakeSolution take;
    MockRefuteBlocked refute;
    MockUnfoldOne unfold;
    MockCheckLastUnfoldOnFocus last_on_focus;
    pud_lodestar_search_stepper<
        MockClearSolution, MockCountSolutions, MockGetPendingConflicts, MockSetPendingConflicts, MockGetStopped,
        MockSetStopped, MockGetFocusUnfolds, MockSetFocusUnfolds, MockTakeSolution, MockRefuteBlocked, MockUnfoldOne,
        MockCheckLastUnfoldOnFocus>
        stepper{
            clear, counted, pending, set_pending, stopped, set_stopped, focus_unfolds, set_focus_unfolds, take,
            refute, unfold, last_on_focus, 2};
};

}

TEST_F(PudLodestarSearchStepperTest, AStoppedSearchDoesNotTick) {
    EXPECT_CALL(clear, clear());
    EXPECT_CALL(stopped, get_stopped()).WillOnce(Return(true));
    EXPECT_CALL(take, take).Times(0);
    EXPECT_FALSE(stepper.step());
}

TEST_F(PudLodestarSearchStepperTest, APendingConflictIsEmittedWithoutSearching) {
    InSequence order;
    EXPECT_CALL(clear, clear());
    EXPECT_CALL(stopped, get_stopped()).WillOnce(Return(false));
    EXPECT_CALL(pending, get()).WillOnce(Return(2u));
    EXPECT_CALL(set_pending, set(1u));
    EXPECT_CALL(take, take).Times(0);
    EXPECT_TRUE(stepper.step());
}

TEST_F(PudLodestarSearchStepperTest, ATakenSolutionEndsTheTick) {
    EXPECT_CALL(clear, clear());
    EXPECT_CALL(stopped, get_stopped()).WillOnce(Return(false));
    EXPECT_CALL(pending, get()).WillOnce(Return(0u));
    EXPECT_CALL(take, take()).WillOnce(Return(true));
    EXPECT_CALL(refute, refute_blocked).Times(0);
    EXPECT_TRUE(stepper.step());
}

TEST_F(PudLodestarSearchStepperTest, ABlockedRefutationEndsTheTick) {
    EXPECT_CALL(clear, clear());
    EXPECT_CALL(stopped, get_stopped()).WillOnce(Return(false));
    EXPECT_CALL(pending, get()).WillOnce(Return(0u));
    EXPECT_CALL(take, take()).WillOnce(Return(false));
    EXPECT_CALL(refute, refute_blocked()).WillOnce(Return(true));
    EXPECT_CALL(unfold, unfold_one).Times(0);
    EXPECT_TRUE(stepper.step());
}

TEST_F(PudLodestarSearchStepperTest, SpendingTheBudgetWithNoSolutionsReturnsOneMoreTick) {
    EXPECT_CALL(clear, clear());
    EXPECT_CALL(stopped, get_stopped()).WillOnce(Return(false));
    EXPECT_CALL(pending, get()).WillOnce(Return(0u));
    EXPECT_CALL(take, take()).WillOnce(Return(false));
    EXPECT_CALL(refute, refute_blocked()).WillOnce(Return(false));
    EXPECT_CALL(focus_unfolds, get_focus_unfolds()).WillOnce(Return(2u));
    EXPECT_CALL(set_stopped, set_stopped(true));
    EXPECT_CALL(counted, solution_count()).WillOnce(Return(0u));
    EXPECT_TRUE(stepper.step());
}

TEST_F(PudLodestarSearchStepperTest, SpendingTheBudgetAfterASolutionStops) {
    EXPECT_CALL(clear, clear());
    EXPECT_CALL(stopped, get_stopped()).WillOnce(Return(false));
    EXPECT_CALL(pending, get()).WillOnce(Return(0u));
    EXPECT_CALL(take, take()).WillOnce(Return(false));
    EXPECT_CALL(refute, refute_blocked()).WillOnce(Return(false));
    EXPECT_CALL(focus_unfolds, get_focus_unfolds()).WillOnce(Return(2u));
    EXPECT_CALL(set_stopped, set_stopped(true));
    EXPECT_CALL(counted, solution_count()).WillOnce(Return(1u));
    EXPECT_FALSE(stepper.step());
}

TEST_F(PudLodestarSearchStepperTest, AnUnfoldOutsideTheFocusDoesNotSpendBudget) {
    InSequence order;
    EXPECT_CALL(clear, clear());
    EXPECT_CALL(stopped, get_stopped()).WillOnce(Return(false));
    EXPECT_CALL(pending, get()).WillOnce(Return(0u));
    EXPECT_CALL(take, take()).WillOnce(Return(false));
    EXPECT_CALL(refute, refute_blocked()).WillOnce(Return(false));
    EXPECT_CALL(focus_unfolds, get_focus_unfolds()).WillOnce(Return(0u));
    EXPECT_CALL(unfold, unfold_one()).WillOnce(Return(true));
    EXPECT_CALL(last_on_focus, last_unfold_on_focus()).WillOnce(Return(false));
    EXPECT_CALL(take, take()).WillOnce(Return(true));
    EXPECT_CALL(set_focus_unfolds, set_focus_unfolds).Times(0);
    EXPECT_TRUE(stepper.step());
}

TEST_F(PudLodestarSearchStepperTest, AFocusUnfoldSpendsOneFromTheBudget) {
    InSequence order;
    EXPECT_CALL(clear, clear());
    EXPECT_CALL(stopped, get_stopped()).WillOnce(Return(false));
    EXPECT_CALL(pending, get()).WillOnce(Return(0u));
    EXPECT_CALL(take, take()).WillOnce(Return(false));
    EXPECT_CALL(refute, refute_blocked()).WillOnce(Return(false));
    EXPECT_CALL(focus_unfolds, get_focus_unfolds()).WillOnce(Return(1u));
    EXPECT_CALL(unfold, unfold_one()).WillOnce(Return(true));
    EXPECT_CALL(last_on_focus, last_unfold_on_focus()).WillOnce(Return(true));
    EXPECT_CALL(focus_unfolds, get_focus_unfolds()).WillOnce(Return(1u));
    EXPECT_CALL(set_focus_unfolds, set_focus_unfolds(2u));
    EXPECT_CALL(take, take()).WillOnce(Return(false));
    EXPECT_CALL(refute, refute_blocked()).WillOnce(Return(false));
    EXPECT_CALL(focus_unfolds, get_focus_unfolds()).WillOnce(Return(2u));
    EXPECT_CALL(set_stopped, set_stopped(true));
    EXPECT_CALL(counted, solution_count()).WillOnce(Return(0u));
    EXPECT_TRUE(stepper.step());
}

TEST_F(PudLodestarSearchStepperTest, NothingLeftToUnfoldStops) {
    EXPECT_CALL(clear, clear());
    EXPECT_CALL(stopped, get_stopped()).WillOnce(Return(false));
    EXPECT_CALL(pending, get()).WillOnce(Return(0u));
    EXPECT_CALL(take, take()).WillOnce(Return(false));
    EXPECT_CALL(refute, refute_blocked()).WillOnce(Return(false));
    EXPECT_CALL(focus_unfolds, get_focus_unfolds()).WillOnce(Return(0u));
    EXPECT_CALL(unfold, unfold_one()).WillOnce(Return(false));
    EXPECT_CALL(set_stopped, set_stopped(true));
    EXPECT_FALSE(stepper.step());
}
