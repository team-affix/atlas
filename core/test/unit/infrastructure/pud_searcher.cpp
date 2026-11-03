#include <optional>
#include <set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_searcher.hpp"
#include "pud_test_node.hpp"

using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

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
struct MockUnfoldOne {
    MOCK_METHOD(pud_unfold_outcome, unfold_one, ());
};
struct MockGetLiveFocusLeaves {
    MOCK_METHOD(const std::set<pud_global_node_id>&, live, (), (const));
};

using test_searcher_t = pud_searcher<
    NiceMock<MockClearSolution>, NiceMock<MockTakePendingConflict>, NiceMock<MockTakeSolution>,
    NiceMock<MockUnfoldOne>, NiceMock<MockGetLiveFocusLeaves>>;

// By default nothing is pending or solved, no focus leaf is live, and every unfold succeeds off the focus.
struct PudSearcherTest : public ::testing::Test {
    NiceMock<MockClearSolution> solutions;
    NiceMock<MockTakePendingConflict> conflicts;
    NiceMock<MockTakeSolution> taker;
    NiceMock<MockUnfoldOne> driver;
    NiceMock<MockGetLiveFocusLeaves> focus_leaves;
    std::set<pud_global_node_id> live;

    void SetUp() override {
        ON_CALL(conflicts, try_take()).WillByDefault(Return(false));
        ON_CALL(taker, take()).WillByDefault(Return(false));
        ON_CALL(driver, unfold_one()).WillByDefault(Return(pud_unfold_outcome::off_focus));
        ON_CALL(focus_leaves, live()).WillByDefault(ReturnRef(live));
    }

    test_searcher_t make_searcher(size_t max_unfolds) {
        return test_searcher_t{solutions, conflicts, taker, driver, focus_leaves, max_unfolds};
    }
};

}

TEST_F(PudSearcherTest, ANewSessionIsRunningWithNoUnfolds) {
    const test_searcher_t searcher = make_searcher(4);
    EXPECT_EQ(searcher.stop_reason(), pud_search_stop_reason::running);
    EXPECT_EQ(searcher.unfold_count(), 0u);
}

TEST_F(PudSearcherTest, NothingRunsUntilTheFirstStepIsAskedFor) {
    test_searcher_t searcher = make_searcher(4);
    EXPECT_CALL(solutions, clear()).Times(0);
    EXPECT_CALL(driver, unfold_one()).Times(0);
    coroutine<pud_search_step, void> steps = searcher.search();
}

TEST_F(PudSearcherTest, EachUnfoldIsItsOwnStep) {
    test_searcher_t searcher = make_searcher(4);
    coroutine<pud_search_step, void> steps = searcher.search();
    EXPECT_CALL(driver, unfold_one()).Times(1);
    EXPECT_EQ(steps.next(), pud_search_step::unfolded);
    EXPECT_EQ(searcher.unfold_count(), 1u);
    EXPECT_CALL(driver, unfold_one()).Times(1);
    EXPECT_EQ(steps.next(), pud_search_step::unfolded);
    EXPECT_EQ(searcher.unfold_count(), 2u);
}

TEST_F(PudSearcherTest, EveryStepFirstClearsTheLastSolution) {
    test_searcher_t searcher = make_searcher(4);
    coroutine<pud_search_step, void> steps = searcher.search();
    InSequence order;
    EXPECT_CALL(solutions, clear());
    EXPECT_CALL(driver, unfold_one());
    EXPECT_CALL(solutions, clear());
    EXPECT_CALL(driver, unfold_one());
    steps.next();
    steps.next();
}

TEST_F(PudSearcherTest, APendingConflictIsAStepWithoutSearching) {
    test_searcher_t searcher = make_searcher(4);
    EXPECT_CALL(conflicts, try_take()).WillOnce(Return(true)).WillRepeatedly(Return(false));
    coroutine<pud_search_step, void> steps = searcher.search();
    EXPECT_CALL(taker, take()).Times(0);
    EXPECT_EQ(steps.next(), pud_search_step::refuted);
}

TEST_F(PudSearcherTest, ATakenSolutionIsAStepBeforeAnyUnfold) {
    test_searcher_t searcher = make_searcher(4);
    EXPECT_CALL(taker, take()).WillOnce(Return(true)).WillRepeatedly(Return(false));
    coroutine<pud_search_step, void> steps = searcher.search();
    EXPECT_CALL(driver, unfold_one()).Times(0);
    EXPECT_EQ(steps.next(), pud_search_step::solved);
    EXPECT_EQ(searcher.unfold_count(), 0u);
}

TEST_F(PudSearcherTest, EveryUnfoldOnOrOffTheFocusSpendsTheBudget) {
    test_searcher_t searcher = make_searcher(2);
    EXPECT_CALL(driver, unfold_one())
        .WillOnce(Return(pud_unfold_outcome::off_focus))
        .WillOnce(Return(pud_unfold_outcome::on_focus));
    coroutine<pud_search_step, void> steps = searcher.search();
    EXPECT_EQ(steps.next(), pud_search_step::unfolded);
    EXPECT_EQ(steps.next(), pud_search_step::unfolded);
    EXPECT_EQ(steps.next(), std::nullopt);
    EXPECT_EQ(searcher.stop_reason(), pud_search_stop_reason::budget_spent);
    EXPECT_TRUE(steps.done());
}

TEST_F(PudSearcherTest, ABudgetOfZeroUnfoldsNothing) {
    test_searcher_t searcher = make_searcher(0);
    EXPECT_CALL(driver, unfold_one()).Times(0);
    coroutine<pud_search_step, void> steps = searcher.search();
    EXPECT_EQ(steps.next(), std::nullopt);
    EXPECT_EQ(searcher.stop_reason(), pud_search_stop_reason::budget_spent);
}

TEST_F(PudSearcherTest, NoUnfoldWithNoOpenFocusLeafIsARefutation) {
    test_searcher_t searcher = make_searcher(4);
    ON_CALL(driver, unfold_one()).WillByDefault(Return(pud_unfold_outcome::none));
    coroutine<pud_search_step, void> steps = searcher.search();
    EXPECT_EQ(steps.next(), std::nullopt);
    EXPECT_EQ(searcher.stop_reason(), pud_search_stop_reason::refuted);
}

TEST_F(PudSearcherTest, NoUnfoldWhileAFocusLeafIsStillOpenIsExhaustion) {
    test_searcher_t searcher = make_searcher(4);
    ON_CALL(driver, unfold_one()).WillByDefault(Return(pud_unfold_outcome::none));
    live.insert(pud_test_node(3));
    coroutine<pud_search_step, void> steps = searcher.search();
    EXPECT_EQ(steps.next(), std::nullopt);
    EXPECT_EQ(searcher.stop_reason(), pud_search_stop_reason::exhausted);
}
