// Integration: a real dbuct_frame_hub driving EVERY real framed store, including
// the real dbuct_mhu_elimination_generator and real dbuct_cdcl_elimination_generator.
//
// Production camps a solver frame (push_solver_frame) and later unwinds it
// (pop_solver_frame). The obligation is that ONE round trip returns every store
// to its pre-camp state -- not each store in isolation, which the per-store unit
// tests already cover, but all of them together off a single hub call. The unit
// dbuct_frame_hub test substitutes fakes for the MHU and CDCL and drives real
// state for goal_exprs only, so a store that is pushed but never popped (or
// popped in the wrong order relative to the avoidance boundary) is invisible
// there.
//
// The object graph mirrors dbuct_ridge_manifest exactly, including the fact that
// a single bind_map instance is shared between the MHU and the hub's bind-map
// slot. The only mock is the MCTS frame depth, which belongs to the MCTS sim --
// outside this slice.

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

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
#include "infrastructure/dbuct_srt_active_goals.hpp"
#include "infrastructure/dbuct_unit_goals.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/pool_allocator.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/solver_frame_depth_tracker.hpp"
#include "infrastructure/unifier.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "value_objects/lineage.hpp"
#include "functor_fixture.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::ReturnPointee;

namespace {

// The MCTS frame depth is owned by the dbuct sim, outside this slice.
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

bool run_unify(unifier_t& u, framed_expr lhs, framed_expr rhs) {
    auto task = u.unify(lhs, rhs);
    while (!task.done())
        task.resume();
    return task.result();
}

}  // namespace

struct DbuctFrameHubRoundTripIntegrationTest : public ::testing::Test {
    test_functors functors;

    size_t mcts_frame_depth = 1;
    NiceMock<MockGetMctsFrameDepth> get_mcts_frame_depth;

    globalizer g;
    bind_map_t bind_map{g};
    bind_map_factory_t bind_map_factory{g};
    local_bind_map_pool_t bind_map_pool;
    unifier_factory_t unifier_factory_{g};
    unifier_t unifier_{g, &bind_map};

    lineage_pool lineage_pool_;
    expr_pool expr_pool_;
    ra_rule_id_set_factory rule_factory;

    solver_frame_depth_tracker solver_frame_depth_tracker_;
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

    mhu_t mhu{bind_map, bind_map, lineage_pool_, expr_pool_,
              bind_map_pool, bind_map_pool, bind_map_pool,
              bind_map_factory, unifier_factory_, goal_candidate_rules};
    cdcl_t cdcl{chosen_goal_candidates, avoidance_unit_boundary, decision_memory,
                avoidance_unit_boundary, avoidance_unit_boundary, avoidance_unit_boundary};

    hub_t hub{solver_frame_depth_tracker_, solver_frame_depth_tracker_,
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

    // Decision lineages whose grandparent is null, so the real nearest_decision
    // (seeded with {nullptr, nullptr}) can answer log_decision's lookup.
    goal_lineage g1{nullptr, 0};
    goal_lineage g2{nullptr, 1};
    resolution_lineage d1{&g1, 0};
    resolution_lineage d2{&g2, 0};

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr func_f{expr::functor{functors.id("f"), {}}};
    expr func_g{expr::functor{functors.id("g"), {}}};

    void SetUp() override {
        ON_CALL(get_mcts_frame_depth, size())
            .WillByDefault(ReturnPointee(&mcts_frame_depth));
    }

    std::vector<const resolution_lineage*> pop_solver_frame() {
        return collect_elims(hub.pop_solver_frame());
    }

    const expr* whnf(const expr* e) { return bind_map.whnf({e, 0}).skeleton; }
};

TEST_F(DbuctFrameHubRoundTripIntegrationTest, PushPopRestoresEveryStoreToPreCampState) {
    const goal_lineage* base_goal = lineage_pool_.make_goal_lineage(nullptr, 0);
    const goal_lineage* camped_goal = lineage_pool_.make_goal_lineage(nullptr, 1);
    const resolution_lineage* camped_rl =
        lineage_pool_.make_resolution_lineage(camped_goal, rule_id{0});

    // Pre-camp state that must survive the round trip untouched.
    goal_exprs.set(base_goal, framed_expr{&func_f, 0});
    srt_active_goals.insert_active_goal(base_goal);
    srt_active_goals.flush_srt_goal_batch();
    const uint32_t base_offset = frame_allocator.peek();

    hub.push_solver_frame();

    // Mutate every store the hub is responsible for.
    goal_exprs.set(camped_goal, framed_expr{&func_g, 0});
    goal_candidate_rules.insert(camped_goal);
    goal_candidate_rules.link_goal_candidate(camped_goal, rule_id{7});
    chosen_goal_candidates.set(camped_goal, rule_id{7});
    decision_memory.record_decision(camped_rl);
    resolution_memory.record_resolution(camped_rl);
    unit_goals.push(camped_goal);
    candidate_frame_offsets.set(camped_rl, 42);
    frame_allocator.bump(16);
    nearest_decision.note_decision_resolution(camped_rl);
    elimination_backlog.insert_backlogged_elimination(camped_rl);
    avoidance_unit_boundary.log_decision(&d1);
    srt_active_goals.insert_active_goal(camped_goal);
    srt_active_goals.flush_srt_goal_batch();
    bind_map.bind(0, framed_expr{&func_f, 0});
    ASSERT_TRUE(mhu.try_add_head(camped_rl, {&var0, 0}, {&func_f, 0}));

    ASSERT_EQ(solver_frame_depth_tracker_.solver_frame_depth(), 2u);

    pop_solver_frame();

    EXPECT_EQ(solver_frame_depth_tracker_.solver_frame_depth(), 1u);
    EXPECT_THROW(goal_exprs.get(camped_goal), std::out_of_range);
    EXPECT_THROW(goal_candidate_rules.get(camped_goal), std::out_of_range);
    EXPECT_EQ(chosen_goal_candidates.try_get(camped_goal), std::nullopt);
    EXPECT_EQ(decision_memory.count(), 0u);
    EXPECT_EQ(resolution_memory.get_resolution_count(), 0u);
    EXPECT_EQ(unit_goals.pop(), std::nullopt);
    EXPECT_THROW(candidate_frame_offsets.get(camped_rl), std::out_of_range);
    EXPECT_EQ(frame_allocator.peek(), base_offset);
    EXPECT_THROW(nearest_decision.get_nearest_decision(camped_rl), std::out_of_range);
    EXPECT_FALSE(elimination_backlog.is_backlogged_elimination(camped_rl));
    EXPECT_EQ(avoidance_unit_boundary.get_ultimate_decision(), nullptr);
    EXPECT_FALSE(srt_active_goals.is_active_goal(camped_goal));
    EXPECT_EQ(whnf(&var0), &var0);
    EXPECT_THROW(collect_elims(mhu.constrain(camped_rl)), std::out_of_range);

    // Pre-camp state is untouched.
    EXPECT_EQ(goal_exprs.get(base_goal).skeleton, &func_f);
    EXPECT_TRUE(srt_active_goals.is_active_goal(base_goal));
    EXPECT_EQ(srt_active_goals.active_goals_size(), 1u);
}

TEST_F(DbuctFrameHubRoundTripIntegrationTest, PopRestoresBindMapWhnfAfterUnifyInCampedFrame) {
    // Real unification writes through the journaling bind map; the camp must
    // unwind those bindings, including whnf's path-compression write-back.
    ASSERT_TRUE(run_unify(unifier_, {&var1, 0}, {&var0, 0}));
    ASSERT_EQ(whnf(&var1), &var0);

    hub.push_solver_frame();
    ASSERT_TRUE(run_unify(unifier_, {&var0, 0}, {&func_f, 0}));
    ASSERT_EQ(whnf(&var1), &func_f);

    pop_solver_frame();

    EXPECT_EQ(whnf(&var0), &var0);
    EXPECT_EQ(whnf(&var1), &var0);
}

TEST_F(DbuctFrameHubRoundTripIntegrationTest, PopRestoresMhuHeadsAndRepMapsAfterConstrain) {
    // constrain() rewrites the MHU's head map, its representative/lineage maps,
    // each head's local bind map, AND the shared bind map. Those undo actions are
    // logged in production but no existing test pops a frame containing them.
    // Replaying the identical constrain after the pop is the end-state assertion:
    // it can only reproduce the same elimination if every one of those rewound.
    const goal_lineage* gl_a = lineage_pool_.make_goal_lineage(nullptr, 0);
    const goal_lineage* gl_b = lineage_pool_.make_goal_lineage(nullptr, 1);
    const resolution_lineage* rl_a = lineage_pool_.make_resolution_lineage(gl_a, rule_id{0});
    const resolution_lineage* rl_b = lineage_pool_.make_resolution_lineage(gl_b, rule_id{0});
    goal_candidate_rules.insert(gl_a);
    goal_candidate_rules.insert(gl_b);
    goal_candidate_rules.link_goal_candidate(gl_a, rule_id{0});
    goal_candidate_rules.link_goal_candidate(gl_b, rule_id{0});

    // Two heads sharing representative var0 with incompatible functors.
    ASSERT_TRUE(mhu.try_add_head(rl_a, {&var0, 0}, {&func_f, 0}));
    ASSERT_TRUE(mhu.try_add_head(rl_b, {&var0, 0}, {&func_g, 0}));

    hub.push_solver_frame();
    ASSERT_THAT(collect_elims(mhu.constrain(rl_a)), ElementsAre(rl_b));
    ASSERT_EQ(whnf(&var0), &func_f);

    pop_solver_frame();

    EXPECT_EQ(whnf(&var0), &var0);
    EXPECT_THAT(collect_elims(mhu.constrain(rl_a)), ElementsAre(rl_b));
}

TEST_F(DbuctFrameHubRoundTripIntegrationTest, PopRestoresCandidateFrameOffsetsAndBumpAllocator) {
    // Candidate activation bumps the allocator and records the resulting offset;
    // the two must unwind together or later unification reads the wrong frame.
    const goal_lineage* gl = lineage_pool_.make_goal_lineage(nullptr, 0);
    const resolution_lineage* rl_first = lineage_pool_.make_resolution_lineage(gl, rule_id{0});
    const resolution_lineage* rl_camped = lineage_pool_.make_resolution_lineage(gl, rule_id{1});

    const uint32_t first_offset = frame_allocator.bump(8);
    candidate_frame_offsets.set(rl_first, first_offset);
    const uint32_t offset_before_camp = frame_allocator.peek();

    hub.push_solver_frame();
    const uint32_t camped_offset = frame_allocator.bump(8);
    candidate_frame_offsets.set(rl_camped, camped_offset);
    ASSERT_EQ(candidate_frame_offsets.get(rl_camped), camped_offset);
    ASSERT_NE(camped_offset, first_offset);

    pop_solver_frame();

    EXPECT_EQ(frame_allocator.peek(), offset_before_camp);
    EXPECT_THROW(candidate_frame_offsets.get(rl_camped), std::out_of_range);
    EXPECT_EQ(candidate_frame_offsets.get(rl_first), first_offset);

    // The reclaimed offset is handed out again, proving the bump really rewound.
    EXPECT_EQ(frame_allocator.bump(8), camped_offset);
}

TEST_F(DbuctFrameHubRoundTripIntegrationTest, NestedCampFramesUnwindIndependently) {
    const goal_lineage* outer = lineage_pool_.make_goal_lineage(nullptr, 0);
    const goal_lineage* inner = lineage_pool_.make_goal_lineage(nullptr, 1);

    hub.push_solver_frame();
    goal_exprs.set(outer, framed_expr{&func_f, 0});
    bind_map.bind(0, framed_expr{&func_f, 0});
    srt_active_goals.insert_active_goal(outer);
    srt_active_goals.flush_srt_goal_batch();

    hub.push_solver_frame();
    goal_exprs.set(inner, framed_expr{&func_g, 0});
    bind_map.bind(1, framed_expr{&func_g, 0});
    srt_active_goals.insert_active_goal(inner);
    srt_active_goals.flush_srt_goal_batch();
    ASSERT_EQ(solver_frame_depth_tracker_.solver_frame_depth(), 3u);

    pop_solver_frame();

    EXPECT_EQ(solver_frame_depth_tracker_.solver_frame_depth(), 2u);
    EXPECT_THROW(goal_exprs.get(inner), std::out_of_range);
    EXPECT_EQ(whnf(&var1), &var1);
    EXPECT_FALSE(srt_active_goals.is_active_goal(inner));
    EXPECT_EQ(goal_exprs.get(outer).skeleton, &func_f);
    EXPECT_EQ(whnf(&var0), &func_f);
    EXPECT_TRUE(srt_active_goals.is_active_goal(outer));

    pop_solver_frame();

    EXPECT_EQ(solver_frame_depth_tracker_.solver_frame_depth(), 1u);
    EXPECT_THROW(goal_exprs.get(outer), std::out_of_range);
    EXPECT_EQ(whnf(&var0), &var0);
    EXPECT_FALSE(srt_active_goals.is_active_goal(outer));
    EXPECT_EQ(srt_active_goals.active_goals_size(), 0u);
}

TEST_F(DbuctFrameHubRoundTripIntegrationTest, PopYieldsCdclEliminationsAfterAvoidanceBoundaryPop) {
    // The hub pops the avoidance boundary BEFORE the CDCL generator so that
    // pop_frame's emit-vs-arm test reads the parent frame's ultimate MCTS depth.
    // With a real boundary and a real decision memory driving the lemma, a hub
    // that popped CDCL first would arm the avoidance instead of emitting it.
    mcts_frame_depth = 1;
    decision_memory.record_decision(&d1);
    avoidance_unit_boundary.log_decision(&d1);

    hub.push_solver_frame();
    mcts_frame_depth = 2;
    decision_memory.record_decision(&d2);
    avoidance_unit_boundary.log_decision(&d2);
    cdcl.learn();

    EXPECT_THAT(pop_solver_frame(), ElementsAre(&d2));
    EXPECT_EQ(avoidance_unit_boundary.get_ultimate_decision(), &d1);
    EXPECT_EQ(avoidance_unit_boundary.get_ultimate_mcts_frame_depth(), 1u);
}
