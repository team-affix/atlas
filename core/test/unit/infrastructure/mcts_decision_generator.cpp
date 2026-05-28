// MCTS decision generator: builds choice vectors from active goals and candidate
// rules, delegates to monte_carlo::simulation, then interns the resolution. With a
// single choice at each stage the outcome is fully determined.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include "../../../core/hpp/infrastructure/mcts_decision_generator.hpp"
#include "../../../core/hpp/infrastructure/rule_set.hpp"
#include "../../../core/hpp/interfaces/i_make_resolution_lineage.hpp"
#include "../../../core/hpp/interfaces/i_iterate_active_goals.hpp"
#include "../../../core/hpp/interfaces/i_active_goals_size.hpp"
#include "../../../core/hpp/interfaces/i_get_goal_candidate_rules.hpp"
#include "../../../mcts/include/mcts.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

namespace {

state_machine<const goal_lineage*> single_goal(const goal_lineage* gl) {
    co_yield gl;
}

}  // namespace

struct MockMakeResolutionLineage : public i_make_resolution_lineage {
    MOCK_METHOD((const resolution_lineage*), make, (const goal_lineage*, rule_id), (override));
};

struct MockIterateActiveGoals : public i_iterate_active_goals {
    MOCK_METHOD(state_machine<const goal_lineage*>, iterate_active_goals, (), (const, override));
};

struct MockActiveGoalsSize : public i_active_goals_size {
    MOCK_METHOD(size_t, active_goals_size, (), (const, override));
};

struct MockGetGoalCandidateRules : public i_get_goal_candidate_rules {
    MOCK_METHOD(i_rule_set&, get, (const goal_lineage*), (override));
    MOCK_METHOD(const i_rule_set&, get, (const goal_lineage*), (const, override));
};

struct MctsDecisionGeneratorTest : public ::testing::Test {
    MockMakeResolutionLineage lp;
    MockIterateActiveGoals iterate_active_goals;
    MockActiveGoalsSize active_goals_size;
    MockGetGoalCandidateRules ggcr;
    monte_carlo::tree_node<mcts_choice> root;
    std::mt19937 rng{0};
    monte_carlo::simulation<mcts_choice, std::mt19937> sim{root, 1.0, rng};
    mcts_decision_generator generator{lp, iterate_active_goals, active_goals_size, ggcr, sim};

    expr goal_e{expr::var{0}};
    expr head{expr::var{1}};
    rule r{&head, {}};
    goal_lineage gl{nullptr, &goal_e};
    resolution_lineage expected_rl{&gl, 0};
    rule_set candidates;
};

TEST_F(MctsDecisionGeneratorTest, GenerateResolvesChosenGoalAndRule) {
    static constexpr rule_id kRule = 0;
    candidates.insert(kRule);
    EXPECT_CALL(active_goals_size, active_goals_size()).WillOnce(Return(1));
    EXPECT_CALL(iterate_active_goals, iterate_active_goals())
        .WillOnce([&] { return single_goal(&gl); });
    EXPECT_CALL(ggcr, get(&gl)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(lp, make(&gl, kRule)).WillOnce(Return(&expected_rl));
    EXPECT_EQ(generator.generate(), &expected_rl);
}
