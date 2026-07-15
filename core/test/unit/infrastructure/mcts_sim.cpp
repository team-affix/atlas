// mcts_sim: owns and drives the internal Monte Carlo simulation only.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include <set>
#include <vector>
#include "infrastructure/mcts_sim.hpp"
#include "value_objects/mcts_choice.hpp"
#include "uniform_value_delta.hpp"

struct MockMakeResolutionLineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id));
};

using test_value_delta_t = monte_carlo::uniform_value_delta<double>;
using test_mcts_sim_t = mcts_sim<test_value_delta_t, MockMakeResolutionLineage>;

struct MctsSimTest : public ::testing::Test {
    static constexpr double kExplorationConstant = 1.414;

    test_value_delta_t value_delta;
    testing::NiceMock<MockMakeResolutionLineage> make_resolution_lineage;
    std::mt19937 rng{42};
    std::set<resolution_lineage> resolution_storage;

    test_mcts_sim_t sim{value_delta, make_resolution_lineage, rng, kExplorationConstant};

    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};

    void SetUp() override {
        ON_CALL(make_resolution_lineage, make_resolution_lineage(testing::_, testing::_))
            .WillByDefault([this](const goal_lineage* gl, rule_id rid) {
                auto [it, _] = resolution_storage.emplace(gl, rid);
                return &*it;
            });
    }
};

TEST_F(MctsSimTest, SetUpThenTearDownRunsWithoutSolverStateCollaborators) {
    value_delta.set_value(-1.0);
    sim.set_up();
    sim.tear_down();
}

TEST_F(MctsSimTest, ChooseAfterSetUpReturnsOneOfSuppliedGoalChoices) {
    sim.set_up();
    std::vector<mcts_choice> choices{mcts_choice{&gl0}, mcts_choice{&gl1}};
    mcts_choice picked = sim.choose(choices, false);
    const auto* picked_gl = std::get_if<const goal_lineage*>(&picked);
    ASSERT_NE(picked_gl, nullptr);
    EXPECT_TRUE(*picked_gl == &gl0 || *picked_gl == &gl1);
    value_delta.set_value(0.0);
    sim.tear_down();
}

TEST_F(MctsSimTest, ChooseAfterSetUpReturnsOneOfSuppliedRuleChoices) {
    sim.set_up();
    std::vector<mcts_choice> choices{mcts_choice{rule_id{0}}, mcts_choice{rule_id{1}}};
    mcts_choice picked = sim.choose(choices, false);
    const auto* picked_rule = std::get_if<rule_id>(&picked);
    ASSERT_NE(picked_rule, nullptr);
    EXPECT_TRUE(*picked_rule == 0 || *picked_rule == 1);
    value_delta.set_value(0.0);
    sim.tear_down();
}

TEST_F(MctsSimTest, MultipleRolloutChoosesStayWithinChoiceSet) {
    sim.set_up();
    std::vector<mcts_choice> choices{mcts_choice{&gl0}, mcts_choice{&gl1}};
    for (int i = 0; i < 10; ++i) {
        mcts_choice picked = sim.choose(choices, false);
        const auto* picked_gl = std::get_if<const goal_lineage*>(&picked);
        ASSERT_NE(picked_gl, nullptr);
        EXPECT_TRUE(*picked_gl == &gl0 || *picked_gl == &gl1);
    }
    value_delta.set_value(0.0);
    sim.tear_down();
}

TEST_F(MctsSimTest, FullLifecycleSetUpChooseTearDownTwice) {
    std::vector<mcts_choice> choices{mcts_choice{rule_id{0}}, mcts_choice{rule_id{1}}};
    for (int cycle = 0; cycle < 2; ++cycle) {
        sim.set_up();
        mcts_choice picked = sim.choose(choices, false);
        EXPECT_TRUE(std::holds_alternative<rule_id>(picked));
        value_delta.set_value(-static_cast<double>(cycle));
        sim.tear_down();
    }
}
