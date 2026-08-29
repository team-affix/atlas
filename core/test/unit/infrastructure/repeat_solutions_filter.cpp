// repeat_solutions_filter: overlays inner run(). On solved, derives the
// resolution lemma once; a repeat becomes conflicted, else pin then remember.

#include <unordered_set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/repeat_solutions_filter.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/sim_termination.hpp"

using ::testing::_;
using ::testing::Return;

struct MockRunSim {
    MOCK_METHOD(sim_termination, run, ());
};

struct MockDeriveResolutionLemma {
    MOCK_METHOD(lemma, derive_resolution_lemma, (), (const));
};

struct MockIsRepeatSolution {
    MOCK_METHOD(bool, is_repeat_solution, (const lemma&), (const));
};

struct MockRememberSolution {
    MOCK_METHOD(void, remember_solution, (const lemma&));
};

struct MockPinResolutionLineage {
    MOCK_METHOD(void, pin, (const resolution_lineage*));
};

using test_repeat_solutions_filter_t = repeat_solutions_filter<
    MockRunSim, MockDeriveResolutionLemma,
    MockIsRepeatSolution, MockRememberSolution,
    MockPinResolutionLineage>;

struct RepeatSolutionsFilterTest : public ::testing::Test {
    MockRunSim run_sim;
    MockDeriveResolutionLemma derive_resolution_lemma;
    MockIsRepeatSolution is_repeat_solution;
    MockRememberSolution remember_solution;
    MockPinResolutionLineage pin_resolution_lineage;
    test_repeat_solutions_filter_t filter;
    resolution_lineage rl0{nullptr, 0};
    resolution_lineage rl1{nullptr, 1};

    RepeatSolutionsFilterTest()
        : filter(run_sim, derive_resolution_lemma,
                 is_repeat_solution, remember_solution,
                 pin_resolution_lineage)
    {}
};

TEST_F(RepeatSolutionsFilterTest, PassesThroughConflicted) {
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::conflicted));
    EXPECT_CALL(derive_resolution_lemma, derive_resolution_lemma()).Times(0);
    EXPECT_CALL(is_repeat_solution, is_repeat_solution(_)).Times(0);
    EXPECT_CALL(pin_resolution_lineage, pin(_)).Times(0);
    EXPECT_CALL(remember_solution, remember_solution(_)).Times(0);

    EXPECT_EQ(filter.run(), sim_termination::conflicted);
}

TEST_F(RepeatSolutionsFilterTest, PassesThroughDepthExceeded) {
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::depth_exceeded));
    EXPECT_CALL(derive_resolution_lemma, derive_resolution_lemma()).Times(0);
    EXPECT_CALL(is_repeat_solution, is_repeat_solution(_)).Times(0);
    EXPECT_CALL(pin_resolution_lineage, pin(_)).Times(0);
    EXPECT_CALL(remember_solution, remember_solution(_)).Times(0);

    EXPECT_EQ(filter.run(), sim_termination::depth_exceeded);
}

TEST_F(RepeatSolutionsFilterTest, NovelSolvedPinsThenRemembers) {
    lemma l{{&rl0, &rl1}};
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::solved));
    EXPECT_CALL(derive_resolution_lemma, derive_resolution_lemma()).WillOnce(Return(l));
    EXPECT_CALL(is_repeat_solution, is_repeat_solution(_)).WillOnce(Return(false));
    EXPECT_CALL(pin_resolution_lineage, pin(&rl0));
    EXPECT_CALL(pin_resolution_lineage, pin(&rl1));
    EXPECT_CALL(remember_solution, remember_solution(_));

    EXPECT_EQ(filter.run(), sim_termination::solved);
}

TEST_F(RepeatSolutionsFilterTest, RepeatSolvedReturnsConflictedWithoutPinOrRemember) {
    lemma l{{&rl0}};
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::solved));
    EXPECT_CALL(derive_resolution_lemma, derive_resolution_lemma()).WillOnce(Return(l));
    EXPECT_CALL(is_repeat_solution, is_repeat_solution(_)).WillOnce(Return(true));
    EXPECT_CALL(pin_resolution_lineage, pin(_)).Times(0);
    EXPECT_CALL(remember_solution, remember_solution(_)).Times(0);

    EXPECT_EQ(filter.run(), sim_termination::conflicted);
}
