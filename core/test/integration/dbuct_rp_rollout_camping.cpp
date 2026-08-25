// Integration: the rollout-policy score map under real solver camping.
//
// Every existing rp_* score test (rp_srt_active_goals_topology.cpp and friends)
// uses the NON-dbuct stores and runs before any simulation, so the percolation
// maths is well covered but the journal is not. In production the score map is a
// dbuct_rp_srt_active_goals whose frames are pushed and popped by
// dbuct_rp_heuristic_rollout_frame_hub, which wraps the real dbuct_frame_hub:
// scores are mutated DURING a rollout and must be unwound when the rollout is
// terminated.
//
// The bug this closes is score bleed. If the score map is not unwound in lockstep
// with the base active-goals store, the next rollout inherits scores from a
// branch that was abandoned -- goals ranked by candidate counts they no longer
// have, or worse, a goal the base store still considers active but which has no
// score entry at all, which throws out of scores_.at() mid-rollout.
//
// Object graph mirrors dbuct_ridge_fc_manifest. The only mock is the MCTS frame
// depth, which belongs to the MCTS sim.

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "infrastructure/candidate_deactivator.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/dbuct_avoidance_unit_boundary.hpp"
#include "infrastructure/dbuct_bind_map.hpp"
#include "infrastructure/dbuct_bind_map_factory.hpp"
#include "infrastructure/dbuct_candidate_frame_offsets.hpp"
#include "infrastructure/dbuct_cdcl_elimination_generator.hpp"
#include "infrastructure/dbuct_chosen_goal_candidates.hpp"
#include "infrastructure/dbuct_decision_memory.hpp"
#include "infrastructure/dbuct_elimination_backlog.hpp"
#include "infrastructure/dbuct_frame_bump_allocator.hpp"
#include "infrastructure/dbuct_frame_hub.hpp"
#include "infrastructure/dbuct_goal_candidate_rules.hpp"
#include "infrastructure/dbuct_goal_exprs.hpp"
#include "infrastructure/dbuct_mhu_elimination_generator.hpp"
#include "infrastructure/dbuct_nearest_decision.hpp"
#include "infrastructure/dbuct_resolution_memory.hpp"
#include "infrastructure/dbuct_rp_heuristic_rollout_frame_hub.hpp"
#include "infrastructure/dbuct_rp_srt_active_goals.hpp"
#include "infrastructure/dbuct_srt_active_goals.hpp"
#include "infrastructure/dbuct_unit_goals.hpp"
#include "infrastructure/elimination_router.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/pool_allocator.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/rp_compute_fewer_candidate_goal_value.hpp"
#include "infrastructure/rp_fewer_candidates_elimination_router.hpp"
#include "infrastructure/solver_frame_depth_tracker.hpp"
#include "infrastructure/unifier.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "value_objects/elimination_result.hpp"
#include "value_objects/lineage.hpp"

using ::testing::NiceMock;
using ::testing::ReturnPointee;

namespace {

struct MockGetMctsFrameDepth {
    MOCK_METHOD(size_t, size, (), (const));
};

using bind_map_t = dbuct_bind_map<globalizer>;
using bind_map_factory_t = dbuct_bind_map_factory<globalizer>;
using unifier_t = unifier<globalizer, bind_map_t>;
using unifier_factory_t = unifier_factory<globalizer, bind_map_t>;
using local_bind_map_pool_t = pool_allocator<bind_map_t>;
using boundary_t =
    dbuct_avoidance_unit_boundary<dbuct_nearest_decision, NiceMock<MockGetMctsFrameDepth>>;

using mhu_t = dbuct_mhu_elimination_generator<
    bind_map_t, bind_map_t, bind_map_t,
    local_bind_map_pool_t, local_bind_map_pool_t, local_bind_map_pool_t,
    bind_map_factory_t, unifier_t, unifier_factory_t,
    lineage_pool, expr_pool, dbuct_goal_candidate_rules>;

using cdcl_t = dbuct_cdcl_elimination_generator<
    dbuct_chosen_goal_candidates, boundary_t, dbuct_decision_memory,
    boundary_t, boundary_t, boundary_t>;

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
    mhu_t, mhu_t,
    cdcl_t, cdcl_t>;

using rp_active_goals_t = dbuct_rp_srt_active_goals<
    dbuct_srt_active_goals, dbuct_srt_active_goals,
    dbuct_srt_active_goals, dbuct_srt_active_goals>;
using rp_hub_t = dbuct_rp_heuristic_rollout_frame_hub<
    hub_t, hub_t, rp_active_goals_t, rp_active_goals_t>;

using candidate_deactivator_t =
    candidate_deactivator<dbuct_candidate_frame_offsets, dbuct_goal_candidate_rules>;
using elimination_router_t = elimination_router<
    dbuct_goal_candidate_rules, dbuct_srt_active_goals,
    dbuct_elimination_backlog, candidate_deactivator_t>;
using compute_goal_value_t = rp_compute_fewer_candidate_goal_value<dbuct_goal_candidate_rules>;
using rp_router_t = rp_fewer_candidates_elimination_router<
    elimination_router_t, compute_goal_value_t, rp_active_goals_t>;

std::vector<const resolution_lineage*> collect_elims(
    coroutine<const resolution_lineage*, void> sm) {
    std::vector<const resolution_lineage*> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            out.push_back(sm.consume_yield());
    }
    return out;
}

}  // namespace

struct DbuctRpRolloutCampingIntegrationTest : public ::testing::Test {
    size_t mcts_frame_depth = 1;
    NiceMock<MockGetMctsFrameDepth> get_mcts_frame_depth;

    globalizer g;
    bind_map_t bind_map{g};
    bind_map_factory_t bind_map_factory{g};
    local_bind_map_pool_t bind_map_pool;
    unifier_factory_t unifier_factory_{g};

    lineage_pool pool;
    expr_pool expr_pool_;
    ra_rule_id_set_factory rule_factory;

    solver_frame_depth_tracker depth_tracker;
    dbuct_goal_exprs goal_exprs;
    dbuct_goal_candidate_rules goal_candidate_rules{rule_factory};
    dbuct_chosen_goal_candidates chosen_goal_candidates;
    dbuct_decision_memory decision_memory;
    dbuct_resolution_memory resolution_memory;
    dbuct_unit_goals unit_goals;
    dbuct_candidate_frame_offsets candidate_frame_offsets;
    dbuct_frame_bump_allocator frame_allocator{0};
    dbuct_nearest_decision nearest_decision;
    dbuct_elimination_backlog elimination_backlog;
    boundary_t avoidance_unit_boundary{nearest_decision, get_mcts_frame_depth};
    dbuct_srt_active_goals srt_active_goals;

    mhu_t mhu{bind_map, bind_map, pool, expr_pool_,
              bind_map_pool, bind_map_pool, bind_map_pool,
              bind_map_factory, unifier_factory_, goal_candidate_rules};
    cdcl_t cdcl{chosen_goal_candidates, avoidance_unit_boundary, decision_memory,
                avoidance_unit_boundary, avoidance_unit_boundary, avoidance_unit_boundary};

    hub_t hub{depth_tracker, depth_tracker,
              goal_exprs, goal_exprs,
              goal_candidate_rules, goal_candidate_rules,
              chosen_goal_candidates, chosen_goal_candidates,
              decision_memory, decision_memory,
              resolution_memory, resolution_memory,
              unit_goals, unit_goals,
              candidate_frame_offsets, candidate_frame_offsets,
              frame_allocator, frame_allocator,
              nearest_decision, nearest_decision,
              elimination_backlog, elimination_backlog,
              avoidance_unit_boundary, avoidance_unit_boundary,
              srt_active_goals, srt_active_goals,
              bind_map, bind_map,
              mhu, mhu,
              cdcl, cdcl};

    rp_active_goals_t rp_active_goals{srt_active_goals, srt_active_goals,
                                      srt_active_goals, srt_active_goals};
    rp_hub_t rp_hub{hub, hub, rp_active_goals, rp_active_goals};

    candidate_deactivator_t candidate_deactivator_{candidate_frame_offsets, goal_candidate_rules};
    elimination_router_t elimination_router_{goal_candidate_rules, srt_active_goals,
                                             elimination_backlog, candidate_deactivator_};
    compute_goal_value_t compute_goal_value{goal_candidate_rules};
    rp_router_t rp_router{elimination_router_, compute_goal_value, rp_active_goals};

    void SetUp() override {
        ON_CALL(get_mcts_frame_depth, size())
            .WillByDefault(ReturnPointee(&mcts_frame_depth));
    }

    // Activates a root goal with the given candidate rules, the way
    // initial_goal_activator + candidate_activator would.
    const goal_lineage* activate_root(subgoal_id idx, const std::vector<rule_id>& candidates) {
        const goal_lineage* gl = pool.make_goal_lineage(nullptr, idx);
        goal_candidate_rules.insert(gl);
        for (rule_id rid : candidates) {
            goal_candidate_rules.link_goal_candidate(gl, rid);
            candidate_frame_offsets.set(pool.make_resolution_lineage(gl, rid), 0u);
        }
        rp_active_goals.insert_active_goal(gl);
        srt_active_goals.flush_srt_goal_batch();
        return gl;
    }

    // Expands parent into body_size children, mirroring subgoals_activator +
    // srt_subgoals_activator: activate the batch, link it, then flush.
    std::vector<const goal_lineage*> expand(const goal_lineage* parent, rule_id rid,
                                            size_t body_size) {
        const resolution_lineage* rl = pool.make_resolution_lineage(parent, rid);
        std::vector<const goal_lineage*> children;
        for (size_t i = 0; i < body_size; ++i) {
            const goal_lineage* child =
                pool.make_goal_lineage(rl, static_cast<subgoal_id>(i));
            goal_candidate_rules.insert(child);
            rp_active_goals.insert_active_goal(child);
            children.push_back(child);
        }
        rp_active_goals.link_srt_goal_batch_parent(parent);
        srt_active_goals.flush_srt_goal_batch();
        return children;
    }

    std::vector<const resolution_lineage*> terminate_rollout() {
        return collect_elims(rp_hub.pop_solver_frame());
    }

    bool has_score(const goal_lineage* gl) const {
        try {
            rp_active_goals.get(gl);
            return true;
        } catch (const std::out_of_range&) {
            return false;
        }
    }
};

TEST_F(DbuctRpRolloutCampingIntegrationTest, RolloutScoresRestoredAfterCampTerminate) {
    const goal_lineage* root_a = activate_root(0, {0, 1, 2});
    const goal_lineage* root_b = activate_root(1, {0});
    rp_active_goals.set_active_goal_value(root_a, -3.0);
    rp_active_goals.set_active_goal_value(root_b, -1.0);

    rp_hub.push_solver_frame();
    // A rollout descends: root_a is expanded and re-scored by percolation from its
    // children, and root_b is re-scored directly.
    const std::vector<const goal_lineage*> children = expand(root_a, 0, 2);
    rp_active_goals.set_active_goal_value(children[0], -5.0);
    rp_active_goals.set_active_goal_value(root_b, -9.0);
    ASSERT_NE(rp_active_goals.get(root_a), -3.0);

    terminate_rollout();

    EXPECT_DOUBLE_EQ(rp_active_goals.get(root_a), -3.0);
    EXPECT_DOUBLE_EQ(rp_active_goals.get(root_b), -1.0);
    // Goals that only existed inside the rollout leave no score behind.
    EXPECT_FALSE(has_score(children[0]));
    EXPECT_FALSE(has_score(children[1]));
}

TEST_F(DbuctRpRolloutCampingIntegrationTest, EliminationDuringRolloutRefreshesScoreThenUndoesIt) {
    // The RP router re-scores a goal from its candidate count after every real
    // elimination. Both the candidate set (framed by dbuct_frame_hub) and the
    // score (framed by the RP hub) must come back together, or the next rollout
    // ranks the goal by a candidate count it no longer has.
    const goal_lineage* root = activate_root(0, {0, 1, 2});
    rp_active_goals.set_active_goal_value(root, compute_goal_value.compute_active_goal_value(root));
    ASSERT_DOUBLE_EQ(rp_active_goals.get(root), -3.0);

    rp_hub.push_solver_frame();
    const resolution_lineage* eliminated = pool.make_resolution_lineage(root, 1);
    ASSERT_EQ(rp_router.route(eliminated), elimination_result::eliminated);
    EXPECT_EQ(goal_candidate_rules.get(root).size(), 2u);
    EXPECT_DOUBLE_EQ(rp_active_goals.get(root), -2.0);

    terminate_rollout();

    EXPECT_EQ(goal_candidate_rules.get(root).size(), 3u);
    EXPECT_TRUE(goal_candidate_rules.get(root).contains(1));
    EXPECT_DOUBLE_EQ(rp_active_goals.get(root), -3.0);
}

TEST_F(DbuctRpRolloutCampingIntegrationTest, RpFramePushesAfterSolverFrameAndPopsBefore) {
    // The RP score frame must open and close inside the same camp boundary as the
    // base active-goals frame. Any drift leaves a goal the base store still calls
    // active with no score entry -- scores_.at() throws mid-rollout -- or a stale
    // score for a goal that is gone.
    const goal_lineage* root = activate_root(0, {0});
    rp_active_goals.set_active_goal_value(root, -1.0);

    rp_hub.push_solver_frame();
    const std::vector<const goal_lineage*> depth1 = expand(root, 0, 2);
    rp_hub.push_solver_frame();
    const std::vector<const goal_lineage*> depth2 = expand(depth1[0], 0, 2);
    rp_active_goals.set_active_goal_value(depth2[0], -4.0);

    // Inner camp only.
    terminate_rollout();
    EXPECT_TRUE(srt_active_goals.is_active_goal(depth1[0]));
    EXPECT_TRUE(has_score(depth1[0]));
    EXPECT_FALSE(srt_active_goals.is_active_goal(depth2[0]));
    EXPECT_FALSE(has_score(depth2[0]));

    // Outer camp.
    terminate_rollout();
    EXPECT_TRUE(srt_active_goals.is_active_goal(root));
    EXPECT_TRUE(has_score(root));
    EXPECT_DOUBLE_EQ(rp_active_goals.get(root), -1.0);
    EXPECT_FALSE(srt_active_goals.is_active_goal(depth1[0]));
    EXPECT_FALSE(has_score(depth1[0]));
    EXPECT_EQ(srt_active_goals.active_goals_size(), 1u);
}
