// Integration: the lazy-propagation CDCL store wired to the real recorder stack,
// and then to the whole ridge_lp solver.
//
// The slice that matters here is lp_decision_recorder + resolution_recorder +
// decision_memory + resolution_memory + lp_cdcl_elimination_generator: descent
// and decision bookkeeping must stay in step, because learn() reads the decision
// set the store accumulated rather than anything decision_memory holds.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/decision_memory.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/lp_cdcl_elimination_generator.hpp"
#include "infrastructure/lp_decision_recorder.hpp"
#include "infrastructure/resolution_memory.hpp"
#include "infrastructure/resolution_recorder.hpp"
#include "infrastructure/ridge_lp_manifest.hpp"
#include "infrastructure/ridge_manifest.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lemma.hpp"
#include "functor_fixture.hpp"

namespace {

using recorder_t = resolution_recorder<decision_memory, resolution_memory>;
using lp_recorder_t = lp_decision_recorder<recorder_t, lp_cdcl_elimination_generator,
                                           lp_cdcl_elimination_generator>;

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

class LpCdclFramePropagationIntegrationTest : public ::testing::Test {
protected:
    lp_cdcl_elimination_generator cdcl;
    decision_memory decisions;
    resolution_memory resolutions;
    recorder_t recorder{decisions, resolutions};
    lp_recorder_t lp_recorder{recorder, cdcl, cdcl};

    // One decision as run_sim performs it: record (which descends), then constrain.
    std::vector<const resolution_lineage*> decide(const resolution_lineage* rl) {
        lp_recorder.record_decision_resolution(rl);
        return collect_elims(cdcl.constrain(rl));
    }

    // One unit resolution as run_sim performs it.
    std::vector<const resolution_lineage*> unit(const resolution_lineage* rl) {
        recorder.record_unit_resolution(rl);
        return collect_elims(cdcl.constrain(rl));
    }

    void restart() {
        cdcl.cleanup();
        decisions.clear_recorded_decisions();
        resolutions.clear_recorded_resolutions();
        cdcl.enter();
        collect_elims(cdcl.flush());
    }

    void end_sim() {
        cdcl.learn();
        restart();
    }

    goal_lineage g0{nullptr, 0};
    goal_lineage g1{nullptr, 1};
    goal_lineage g2{nullptr, 2};

    resolution_lineage g0_r0{&g0, 0};
    resolution_lineage g0_r1{&g0, 1};
    resolution_lineage g1_r0{&g1, 0};
    resolution_lineage g2_r0{&g2, 0};
};

TEST_F(LpCdclFramePropagationIntegrationTest, RecorderKeepsDecisionMemoryAndFrameDescentInStep) {
    /*
     * Intent: one call through the recorder both records the decision and moves
     * the store into the child frame, so the learned avoidance covers exactly the
     * decisions decision_memory saw.
     */
    decide(&g0_r0);
    decide(&g1_r0);
    EXPECT_EQ(decisions.count(), 2u);

    end_sim();

    // The avoidance the store built is {g0_r0, g1_r0}: descending the first
    // member leaves the second as a forced elimination.
    EXPECT_THAT(decide(&g0_r0), ::testing::ElementsAre(&g1_r0));
}

TEST_F(LpCdclFramePropagationIntegrationTest, SecondSimDownSamePrefixReusesTheChildCache) {
    /*
     * Intent: the cross-sim invariant. A prefix walked in sim 1 has its reduced
     * copy waiting in sim 2, so re-walking it delivers nothing new and the work
     * resumes from where the child left off.
     */
    decide(&g0_r0);
    decide(&g1_r0);
    decide(&g2_r0);
    end_sim();

    // Sim 2: seed the cache into the {g0_r0} frame. Do not learn -- the current
    // frame is a prefix, not a new conflict.
    EXPECT_THAT(decide(&g0_r0), ::testing::IsEmpty());
    restart();

    // Sim 3: the same edge has nothing left to send, but the child still holds
    // the copy already reduced by g0_r0.
    EXPECT_THAT(decide(&g0_r0), ::testing::IsEmpty());
    EXPECT_THAT(decide(&g1_r0), ::testing::ElementsAre(&g2_r0));
}

TEST_F(LpCdclFramePropagationIntegrationTest, SiblingDecisionNeverReceivesTheAvoidance) {
    /*
     * Intent: a decision that resolves a member's goal another way satisfies the
     * avoidance, so the subtree under it is never given a copy at all.
     */
    decide(&g0_r0);
    decide(&g1_r0);
    end_sim();

    EXPECT_THAT(decide(&g0_r1), ::testing::IsEmpty());
    EXPECT_THAT(unit(&g1_r0), ::testing::IsEmpty());
}

// The whole ridge_lp stack against ridge on a problem that only refutes after
// multi-decision avoidances have been learned and propagated.

class RidgeLpRefutationIntegrationTest : public ::testing::Test {
protected:
    test_functors functors;
    static constexpr size_t kInitialVarCount = 0;
    static constexpr size_t kMaxResolutions = 64;
    static constexpr uint32_t kSeed = 42;
    static constexpr double kExplorationConstant = 1.414;

    db database;
    initial_goal_exprs initial_goals;
    expr_pool saved_expr_pool_;

    /*
     * initial goals: p, q.
     * rules:
     *   0: p :- a.
     *   1: p :- b.
     *   2: a.
     *   3: b.
     *   4: q :- c.
     *   5: q :- d.
     * c and d have no rules, so q is unsatisfiable while p forces a decision
     * first -- which is what produces two-decision avoidances to propagate.
     */
    void build_problem() {
        const expr* p_goal = saved_expr_pool_.make_functor(functors.id("p"), {});
        const expr* q_goal = saved_expr_pool_.make_functor(functors.id("q"), {});
        initial_goals.push(p_goal);
        initial_goals.push(q_goal);

        const expr* p_head_a = saved_expr_pool_.make_functor(functors.id("p"), {});
        const expr* a_body = saved_expr_pool_.make_functor(functors.id("a"), {});
        const expr* p_head_b = saved_expr_pool_.make_functor(functors.id("p"), {});
        const expr* b_body = saved_expr_pool_.make_functor(functors.id("b"), {});
        const expr* a_head = saved_expr_pool_.make_functor(functors.id("a"), {});
        const expr* b_head = saved_expr_pool_.make_functor(functors.id("b"), {});
        const expr* q_head_c = saved_expr_pool_.make_functor(functors.id("q"), {});
        const expr* c_body = saved_expr_pool_.make_functor(functors.id("c"), {});
        const expr* q_head_d = saved_expr_pool_.make_functor(functors.id("q"), {});
        const expr* d_body = saved_expr_pool_.make_functor(functors.id("d"), {});

        database.push(rule{p_head_a, {a_body}});
        database.push(rule{p_head_b, {b_body}});
        database.push(rule{a_head, {}});
        database.push(rule{b_head, {}});
        database.push(rule{q_head_c, {c_body}});
        database.push(rule{q_head_d, {d_body}});
    }
};

TEST_F(RidgeLpRefutationIntegrationTest, RefutesTheSameProblemAsRidge) {
    /*
     * Intent: the lazy-propagation store drives the same search to the same
     * verdict as the watchlist store it replaces.
     */
    build_problem();

    ridge_manifest baseline{database, initial_goals, kInitialVarCount,
                            kMaxResolutions, kSeed, kExplorationConstant};
    while (baseline.driver_.next()) {}
    ASSERT_FALSE(baseline.driver_.solved());

    ridge_lp_manifest manifest{database, initial_goals, kInitialVarCount,
                               kMaxResolutions, kSeed, kExplorationConstant};
    while (manifest.driver_.next()) {}
    EXPECT_FALSE(manifest.driver_.solved());
}

TEST_F(RidgeLpRefutationIntegrationTest, SolvesASatisfiableProblem) {
    /*
     * Intent: propagation prunes without over-pruning -- adding a rule for c
     * makes q reachable and the same stack finds it.
     * extra rule:
     *   6: c.
     */
    build_problem();
    const expr* c_head = saved_expr_pool_.make_functor(functors.id("c"), {});
    database.push(rule{c_head, {}});

    ridge_lp_manifest manifest{database, initial_goals, kInitialVarCount,
                               kMaxResolutions, kSeed, kExplorationConstant};
    while (manifest.driver_.next()) {}
    EXPECT_TRUE(manifest.driver_.solved());
}

}  // namespace
