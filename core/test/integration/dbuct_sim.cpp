// Integration: dbuct_sim with a real frame hub + framed stores.
// Mock only MCTS choose/terminate/depth/backstep and the rule-choice checker.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstddef>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/dbuct_avoidance_unit_boundary.hpp"
#include "infrastructure/dbuct_bind_map.hpp"
#include "infrastructure/dbuct_candidate_frame_offsets.hpp"
#include "infrastructure/dbuct_chosen_goal_candidates.hpp"
#include "infrastructure/dbuct_decision_memory.hpp"
#include "infrastructure/dbuct_elimination_backlog.hpp"
#include "infrastructure/dbuct_frame_bump_allocator.hpp"
#include "infrastructure/dbuct_frame_hub.hpp"
#include "infrastructure/solver_frame_depth_tracker.hpp"
#include "infrastructure/dbuct_goal_candidate_rules.hpp"
#include "infrastructure/dbuct_goal_exprs.hpp"
#include "infrastructure/dbuct_nearest_decision.hpp"
#include "infrastructure/dbuct_resolution_memory.hpp"
#include "infrastructure/dbuct_sim.hpp"
#include "infrastructure/dbuct_srt_active_goals.hpp"
#include "infrastructure/dbuct_unit_goals.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockMctsOps {
    MOCK_METHOD(size_t, depth, (), (const));
    MOCK_METHOD(void, backstep, ());
    MOCK_METHOD(bool, in_rollout, (), (const));
    MOCK_METHOD(mcts_choice, choose,
        (const std::vector<mcts_choice>&, const std::vector<mcts_choice>&));
    MOCK_METHOD(void, terminate, ());
};

struct MockCheckRuleChoice {
    MOCK_METHOD(bool, check_is_rule_choice, (const mcts_choice&), (const));
};

struct yielding_cdcl {
    std::vector<const resolution_lineage*> to_yield;
    int pushes = 0;
    int pops = 0;

    void push_frame() { ++pushes; }

    coroutine<const resolution_lineage*, void> pop_frame() {
        ++pops;
        for (const resolution_lineage* rl : to_yield)
            co_yield rl;
    }
};

struct fake_mhu {
    int pushes = 0;
    int pops = 0;
    void push_frame() { ++pushes; }
    void pop_frame() { ++pops; }
};

using bind_map_t = dbuct_bind_map<globalizer>;
using boundary_t = dbuct_avoidance_unit_boundary<dbuct_nearest_decision, MockMctsOps>;

using hub_t = dbuct_frame_hub<
    solver_frame_depth_tracker, solver_frame_depth_tracker,
    dbuct_goal_exprs, dbuct_goal_exprs,
    dbuct_goal_candidate_rules, dbuct_goal_candidate_rules,
    dbuct_chosen_goal_candidates, dbuct_chosen_goal_candidates,
    dbuct_decision_memory, dbuct_decision_memory,
    dbuct_resolution_memory, dbuct_resolution_memory,
    dbuct_unit_goals, dbuct_unit_goals,
    dbuct_candidate_frame_offsets, dbuct_candidate_frame_offsets,
    dbuct_frame_bump_allocator, dbuct_frame_bump_allocator,
    dbuct_nearest_decision, dbuct_nearest_decision,
    dbuct_elimination_backlog, dbuct_elimination_backlog,
    boundary_t, boundary_t,
    dbuct_srt_active_goals, dbuct_srt_active_goals,
    bind_map_t, bind_map_t,
    fake_mhu, fake_mhu,
    yielding_cdcl, yielding_cdcl>;

using sim_t = dbuct_sim<
    mcts_choice,
    hub_t, hub_t,
    solver_frame_depth_tracker,
    dbuct_decision_memory,
    boundary_t, boundary_t,
    MockMctsOps, MockMctsOps, MockMctsOps, MockMctsOps, MockMctsOps,
    MockCheckRuleChoice>;

struct DbuctSimIntegrationTest : public ::testing::Test {
    NiceMock<MockMctsOps> mcts;
    NiceMock<MockCheckRuleChoice> check_rule_choice;

    globalizer g;
    bind_map_t bind_map{g};
    ra_rule_id_set_factory rule_factory;
    dbuct_goal_exprs goal_exprs;
    dbuct_goal_candidate_rules goal_candidate_rules{rule_factory};
    dbuct_chosen_goal_candidates chosen_goal_candidates;
    dbuct_decision_memory decision_memory;
    dbuct_resolution_memory resolution_memory;
    dbuct_unit_goals unit_goals;
    dbuct_candidate_frame_offsets candidate_frame_offsets;
    dbuct_frame_bump_allocator frame_bump_allocator{0};
    dbuct_nearest_decision nearest_decision;
    dbuct_elimination_backlog elimination_backlog;
    boundary_t avoidance_unit_boundary{nearest_decision, mcts};
    dbuct_srt_active_goals srt_active_goals;
    solver_frame_depth_tracker solver_frame_depth_tracker_;
    fake_mhu mhu;
    yielding_cdcl cdcl;
    hub_t hub;
    sim_t sim;

    goal_lineage gl{nullptr, 0};
    resolution_lineage elim{&gl, 0};

    DbuctSimIntegrationTest()
        : hub(solver_frame_depth_tracker_, solver_frame_depth_tracker_,
              goal_exprs, goal_exprs,
              goal_candidate_rules, goal_candidate_rules,
              chosen_goal_candidates, chosen_goal_candidates,
              decision_memory, decision_memory,
              resolution_memory, resolution_memory,
              unit_goals, unit_goals,
              candidate_frame_offsets, candidate_frame_offsets,
              frame_bump_allocator, frame_bump_allocator,
              nearest_decision, nearest_decision,
              elimination_backlog, elimination_backlog,
              avoidance_unit_boundary, avoidance_unit_boundary,
              srt_active_goals, srt_active_goals,
              bind_map, bind_map,
              mhu, mhu,
              cdcl, cdcl)
        , sim(hub, hub, solver_frame_depth_tracker_,
              decision_memory,
              avoidance_unit_boundary, avoidance_unit_boundary,
              mcts, mcts, mcts, mcts, mcts,
              check_rule_choice) {}
};

}  // namespace

TEST_F(DbuctSimIntegrationTest, RuleChoosePastUltimatePushesVisibleSolverFrame) {
    mcts_choice chosen = rule_id{3};
    std::vector<mcts_choice> choices{chosen};

    EXPECT_CALL(mcts, choose(::testing::_, ::testing::_)).WillOnce(Return(chosen));
    EXPECT_CALL(mcts, depth()).WillRepeatedly(Return(3));
    EXPECT_CALL(check_rule_choice, check_is_rule_choice(chosen)).WillOnce(Return(true));

    EXPECT_EQ(mhu.pushes, 0);
    EXPECT_EQ(sim.choose(choices), chosen);
    // Hub push fans out to framed stores (MHU/CDCL hooks observe it).
    EXPECT_EQ(mhu.pushes, 1);
    EXPECT_EQ(cdcl.pushes, 1);

    goal_exprs.set(&gl, framed_expr{nullptr, 7});
    EXPECT_EQ(goal_exprs.get(&gl).frame_offset, 7u);
}

TEST_F(DbuctSimIntegrationTest, TerminateRestoresStoresAndReturnsPopEliminations) {
    mcts_choice chosen = rule_id{1};
    std::vector<mcts_choice> choices{chosen};
    cdcl.to_yield = {&elim};

    EXPECT_CALL(mcts, choose(::testing::_, ::testing::_)).WillOnce(Return(chosen));
    EXPECT_CALL(check_rule_choice, check_is_rule_choice(chosen)).WillOnce(Return(true));
    // choose at depth 3 (past ultimate 1) → push; log_decision at 4; terminate at 1.
    EXPECT_CALL(mcts, depth())
        .WillOnce(Return(3))
        .WillOnce(Return(4))
        .WillRepeatedly(Return(1));

    EXPECT_EQ(sim.choose(choices), chosen);
    goal_exprs.set(&gl, framed_expr{nullptr, 9});
    EXPECT_EQ(mhu.pushes, 1);

    // Raise ultimate above current MCTS depth so terminate must pop the camped frame.
    // log_decision journals into the pushed aub frame; pop undoes it.
    avoidance_unit_boundary.log_decision(&elim);
    EXPECT_EQ(avoidance_unit_boundary.get_ultimate_mcts_frame_depth(), 4u);

    EXPECT_CALL(mcts, terminate()).Times(1);
    EXPECT_CALL(mcts, backstep()).Times(0);

    std::vector<const resolution_lineage*> got = sim.terminate();
    EXPECT_EQ(got, std::vector<const resolution_lineage*>{&elim});
    EXPECT_EQ(mhu.pops, 1);
    EXPECT_EQ(cdcl.pops, 1);
    EXPECT_THROW(goal_exprs.get(&gl), std::out_of_range);
    EXPECT_EQ(avoidance_unit_boundary.get_ultimate_mcts_frame_depth(), 1u);
}
