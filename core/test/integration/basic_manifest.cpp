// Integration: basic_manifest — wiring, sim lifecycle, cross-tick solver interop.
//
// Harness: enumerate_all_solutions, next_until_refuted. Solver ticks: sm.resume / has_yield / consume_yield.
//
// Bug policy (docs/testing.md): failing tests indicate suspected production bugs
// unless setup/lifetime/key definitions are wrong. Do not delete or weaken tests.

#include <functional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "infrastructure/basic_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/locator.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/trail.hpp"
#include "interfaces/i_push_trail_frame.hpp"
#include "interfaces/i_pop_trail_frame.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"
#include "interfaces/i_record_decision.hpp"
#include "interfaces/i_clear_recorded_decisions.hpp"
#include "interfaces/i_get_decision_count.hpp"
#include "interfaces/i_derive_decision_lemma.hpp"
#include "interfaces/i_set_up_sim.hpp"
#include "interfaces/i_tear_down_sim.hpp"
#include "interfaces/i_run_sim.hpp"
#include "interfaces/i_try_add_mhu_head.hpp"
#include "interfaces/i_clear_mhu_heads.hpp"
#include "interfaces/i_make_goal_lineage.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_pin_goal_lineage.hpp"
#include "interfaces/i_pin_resolution_lineage.hpp"
#include "interfaces/i_trim_unpinned_lineages.hpp"
#include "interfaces/i_learn_avoidance.hpp"
#include "interfaces/i_elimination_generator.hpp"
#include "interfaces/i_bind_map.hpp"
#include "interfaces/i_bind_map_factory.hpp"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_import_expr.hpp"
#include "interfaces/i_clear_bindings.hpp"
#include "interfaces/i_record_resolution.hpp"
#include "interfaces/i_clear_recorded_resolutions.hpp"
#include "interfaces/i_derive_resolution_lemma.hpp"
#include "interfaces/i_get_resolution_count.hpp"
#include "interfaces/i_resolver.hpp"
#include "interfaces/i_elimination_router.hpp"
#include "interfaces/i_generate_decision.hpp"
#include "interfaces/i_solve.hpp"
#include "interfaces/i_check_active_goals_empty.hpp"
#include "value_objects/sim_termination.hpp"
#include "value_objects/lemma.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

namespace {

// ---------------------------------------------------------------------------
// Solver enumeration harness
// ---------------------------------------------------------------------------

using solution = std::vector<const expr*>;

void enumerate_all_solutions(
    i_solve& solver,
    std::set<solution> expected,
    const std::function<solution()>& get_solution) {
    auto sm = solver.solve();
    std::set<solution> visited;
    while (!expected.empty()) {
        solution s;
        do {
            sm.resume();
            ASSERT_TRUE(sm.has_yield()) << "solver stopped before all expected solutions found";
            if (sm.consume_yield() != sim_termination::solved)
                continue;
            s = get_solution();
        } while (visited.count(s));
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
        visited.insert(s);
    }
}

void next_until_refuted(
    i_solve& solver,
    std::set<solution> expected,
    const std::function<solution()>& get_solution) {
    auto sm = solver.solve();
    std::set<solution> visited;
    while (true) {
        sm.resume();
        if (!sm.has_yield())
            break;
        if (sm.consume_yield() != sim_termination::solved)
            continue;
        const solution s = get_solution();
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver refuted before all expected solutions found";
}

}  // namespace

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

struct BasicManifestIntegrationTest : public ::testing::Test {
    static constexpr size_t kMaxResolutions = 32;
    static constexpr uint32_t kSeed = 42;

    db database;
    initial_goal_exprs initial_goals;
    std::vector<expr> expr_storage;

    trail saved_trail_;
    locator saved_loc_;
    expr_pool saved_expr_pool_{bind_saved_loc(saved_trail_, saved_loc_)};

private:
    static locator& bind_saved_loc(trail& t, locator& l) {
        l.bind_as<i_log_to_current_trail_frame>(t);
        return l;
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// Tier W — wiring / shared-instance identity (no sim run)

TEST_F(BasicManifestIntegrationTest, WiringCdclIsLearnAvoidanceNotJoint) {
    /*
     * Intent: cdcl_ is the locator's i_learn_avoidance binding, distinct from joint_.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    EXPECT_EQ(static_cast<i_learn_avoidance *>(&manifest.cdcl_), &manifest.loc_.locate<i_learn_avoidance>());
    EXPECT_NE(static_cast<void*>(&manifest.loc_.locate<i_learn_avoidance>()),
        static_cast<void*>(&manifest.loc_.locate<i_elimination_generator>()));
}

TEST_F(BasicManifestIntegrationTest, WiringJointIsEliminationGeneratorNotCdcl) {
    /*
     * Intent: joint_ is the locator's i_elimination_generator binding.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    EXPECT_EQ(static_cast<i_elimination_generator *>(&manifest.joint_), &manifest.loc_.locate<i_elimination_generator>());
}

TEST_F(BasicManifestIntegrationTest, WiringTrailSharedForPushPopLog) {
    /*
     * Intent: trail_ backs i_push_trail_frame, i_pop_trail_frame, and i_log_to_current_trail_frame.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    EXPECT_EQ(static_cast<i_push_trail_frame *>(&manifest.trail_), &manifest.loc_.locate<i_push_trail_frame>());
    EXPECT_EQ(static_cast<i_pop_trail_frame *>(&manifest.trail_), &manifest.loc_.locate<i_pop_trail_frame>());
    EXPECT_EQ(static_cast<i_log_to_current_trail_frame *>(&manifest.trail_), &manifest.loc_.locate<i_log_to_current_trail_frame>());
}

TEST_F(BasicManifestIntegrationTest, WiringDecisionMemorySharedForRecordCountDerive) {
    /*
     * Intent: decision_memory_ backs record/clear/count/derive decision interfaces.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    EXPECT_EQ(static_cast<i_record_decision *>(&manifest.decision_memory_), &manifest.loc_.locate<i_record_decision>());
    EXPECT_EQ(static_cast<i_clear_recorded_decisions *>(&manifest.decision_memory_), &manifest.loc_.locate<i_clear_recorded_decisions>());
    EXPECT_EQ(static_cast<i_get_decision_count *>(&manifest.decision_memory_), &manifest.loc_.locate<i_get_decision_count>());
    EXPECT_EQ(static_cast<i_derive_decision_lemma *>(&manifest.decision_memory_), &manifest.loc_.locate<i_derive_decision_lemma>());
}

TEST_F(BasicManifestIntegrationTest, WiringSimSharedForSetUpRunTearDown) {
    /*
     * Intent: sim_ backs i_set_up_sim, i_run_sim, and i_tear_down_sim.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    EXPECT_EQ(static_cast<i_set_up_sim *>(&manifest.sim_), &manifest.loc_.locate<i_set_up_sim>());
    EXPECT_EQ(static_cast<i_tear_down_sim *>(&manifest.sim_), &manifest.loc_.locate<i_tear_down_sim>());
    EXPECT_EQ(static_cast<i_run_sim *>(&manifest.sim_), &manifest.loc_.locate<i_run_sim>());
}

TEST_F(BasicManifestIntegrationTest, WiringMhuSharedForHeadOps) {
    /*
     * Intent: mhu_ backs i_try_add_mhu_head and i_clear_mhu_heads.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    EXPECT_EQ(static_cast<i_try_add_mhu_head *>(&manifest.mhu_), &manifest.loc_.locate<i_try_add_mhu_head>());
    EXPECT_EQ(static_cast<i_clear_mhu_heads *>(&manifest.mhu_), &manifest.loc_.locate<i_clear_mhu_heads>());
}

TEST_F(BasicManifestIntegrationTest, WiringLineagePoolSharedForMakePinTrim) {
    /*
     * Intent: lineage_pool_ backs make/pin/trim lineage interfaces.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    EXPECT_EQ(static_cast<i_make_goal_lineage *>(&manifest.lineage_pool_), &manifest.loc_.locate<i_make_goal_lineage>());
    EXPECT_EQ(static_cast<i_make_resolution_lineage *>(&manifest.lineage_pool_), &manifest.loc_.locate<i_make_resolution_lineage>());
    EXPECT_EQ(static_cast<i_pin_goal_lineage *>(&manifest.lineage_pool_), &manifest.loc_.locate<i_pin_goal_lineage>());
    EXPECT_EQ(static_cast<i_pin_resolution_lineage *>(&manifest.lineage_pool_), &manifest.loc_.locate<i_pin_resolution_lineage>());
    EXPECT_EQ(static_cast<i_trim_unpinned_lineages *>(&manifest.lineage_pool_), &manifest.loc_.locate<i_trim_unpinned_lineages>());
}

TEST_F(BasicManifestIntegrationTest, WiringSolverIsISolve) {
    /*
     * Intent: solver_ is the locator's i_solve binding.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    EXPECT_EQ(static_cast<i_solve *>(&manifest.solver_), &manifest.loc_.locate<i_solve>());
}

TEST_F(BasicManifestIntegrationTest, WiringConstructsWithEmptyDbAndNoGoals) {
    /*
     * Intent: basic_manifest constructs without throwing on an empty problem.
     * initial goals: (none)
     * rules: (none)
     */
    EXPECT_NO_THROW(
        (basic_manifest{database, initial_goals, kMaxResolutions, kSeed}));
}

TEST_F(BasicManifestIntegrationTest, WiringResolutionMemorySharedForRecordDerive) {
    /*
     * Intent: resolution_memory_ backs record/clear/count/derive resolution interfaces.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    EXPECT_EQ(static_cast<i_record_resolution *>(&manifest.resolution_memory_), &manifest.loc_.locate<i_record_resolution>());
    EXPECT_EQ(static_cast<i_clear_recorded_resolutions *>(&manifest.resolution_memory_), &manifest.loc_.locate<i_clear_recorded_resolutions>());
    EXPECT_EQ(static_cast<i_derive_resolution_lemma *>(&manifest.resolution_memory_), &manifest.loc_.locate<i_derive_resolution_lemma>());
    EXPECT_EQ(static_cast<i_get_resolution_count *>(&manifest.resolution_memory_), &manifest.loc_.locate<i_get_resolution_count>());
}

TEST_F(BasicManifestIntegrationTest, WiringBindMapSharedForWhnfAndClear) {
    /*
     * Intent: bind_map_ backs i_bind_map and i_clear_bindings.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    EXPECT_EQ(static_cast<i_bind_map *>(&manifest.bind_map_), &manifest.loc_.locate<i_bind_map>());
    EXPECT_EQ(static_cast<i_clear_bindings *>(&manifest.bind_map_), &manifest.loc_.locate<i_clear_bindings>());
}

TEST_F(BasicManifestIntegrationTest, WiringResolverAndRouterBound) {
    /*
     * Intent: resolver_ and elimination_router_ are bound on the locator.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    EXPECT_EQ(static_cast<i_resolver *>(&manifest.resolver_), &manifest.loc_.locate<i_resolver>());
    EXPECT_EQ(static_cast<i_elimination_router *>(&manifest.elimination_router_), &manifest.loc_.locate<i_elimination_router>());
}

TEST_F(BasicManifestIntegrationTest, WiringRandomDecisionGeneratorIsGenerateDecision) {
    /*
     * Intent: random_decision_generator_ is the locator's i_generate_decision binding.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    EXPECT_EQ(static_cast<i_generate_decision *>(&manifest.random_decision_generator_), &manifest.loc_.locate<i_generate_decision>());
}

TEST_F(BasicManifestIntegrationTest, WiringMaxResolutionsStored) {
    /*
     * Intent: constructor stores the max_resolutions budget on the manifest.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    EXPECT_EQ(manifest.max_resolutions_, kMaxResolutions);
}

TEST_F(BasicManifestIntegrationTest, WiringBindMapAndExprPoolSharedViaLocator) {
    /*
     * Intent: bind_map_, bind_map_factory_, and expr_pool_ share locator bindings.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    EXPECT_EQ(static_cast<i_bind_map *>(&manifest.bind_map_), &manifest.loc_.locate<i_bind_map>());
    EXPECT_EQ(static_cast<i_bind_map_factory *>(&manifest.bind_map_factory_), &manifest.loc_.locate<i_bind_map_factory>());
    EXPECT_EQ(static_cast<i_make_functor *>(&manifest.expr_pool_), &manifest.loc_.locate<i_make_functor>());
    EXPECT_EQ(static_cast<i_import_expr *>(&manifest.expr_pool_), &manifest.loc_.locate<i_import_expr>());
}

// Tier L — sim lifecycle + subsystems via manifest.sim_

TEST_F(BasicManifestIntegrationTest, SimLifecycleTrailDepthRestoresAfterEmptyRun) {
    /*
     * Intent: sim set_up/tear_down on an empty problem restores trail depth.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    const size_t depth_before = manifest.trail_.depth();
    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);
    manifest.sim_.tear_down();
    EXPECT_EQ(manifest.trail_.depth(), depth_before);
}

TEST_F(BasicManifestIntegrationTest, SimLifecycleTrailDepthRestoresAfterConflictedRun) {
    /*
     * Intent: trail depth restores after a conflicted sim run with no matching rules.
     * initial goals: f.
     * rules: (none)
     */
    expr goal{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    const size_t depth_before = manifest.trail_.depth();
    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::conflicted);
    manifest.sim_.tear_down();
    EXPECT_EQ(manifest.trail_.depth(), depth_before);
}

TEST_F(BasicManifestIntegrationTest, SimLifecycleTrailDepthRestoresAfterDepthExceededRun) {
    /*
     * Intent: trail depth restores when resolution budget is exceeded on a recursive clause.
     * initial goals: f.
     * rules:
     *   0: f :- f.
     */
    expr goal{expr::functor{"f", {}}};
    expr f_head{expr::functor{"f", {}}};
    expr f_body{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head, {&f_body}});
    static constexpr size_t kLowBudget = 4;
    basic_manifest manifest{database, initial_goals, kLowBudget, kSeed};
    const size_t depth_before = manifest.trail_.depth();
    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::depth_exceeded);
    manifest.sim_.tear_down();
    EXPECT_EQ(manifest.trail_.depth(), depth_before);
}

TEST_F(BasicManifestIntegrationTest, SimLifecycleClearsEphemeralStoresAfterSolvedRun) {
    /*
     * Intent: tear_down clears decision/resolution memory, active goals, and sim bindings.
     * initial goals: f(A, B).  (A, B are fresh logic vars)
     * rules:
     *   0: f(abc, 123).
     */
    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr head{expr::functor{"f", {&abc, &_123}}};
    database.push(rule{&head, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    const uint32_t idx_test = manifest.var_sequencer_.next();
    const expr* test_var = manifest.expr_pool_.make(idx_test);
    initial_goals.push(manifest.expr_pool_.make("f", {test_var,
        manifest.expr_pool_.make(manifest.var_sequencer_.next())}));

    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);
    manifest.sim_.tear_down();

    EXPECT_TRUE(manifest.loc_.locate<i_check_active_goals_empty>().empty());
    EXPECT_EQ(manifest.decision_memory_.count(), 0u);
    EXPECT_EQ(manifest.resolution_memory_.get_resolution_count(), 0u);
    EXPECT_THAT(
        manifest.resolution_memory_.derive_resolution_lemma().get_resolutions(), IsEmpty());
    const expr* whnf = manifest.bind_map_.whnf(test_var);
    ASSERT_TRUE(std::holds_alternative<expr::var>(whnf->content));
    EXPECT_EQ(std::get<expr::var>(whnf->content).index, idx_test);
}

TEST_F(BasicManifestIntegrationTest, SimLifecycleBaseFrameVarsSurviveTearDown) {
    /*
     * Intent: base-frame variable indices and solved bindings persist across sim cycles.
     * initial goals: f(A, B).
     * rules:
     *   0: f(abc, 123).
     */
    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr head{expr::functor{"f", {&abc, &_123}}};
    database.push(rule{&head, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const uint32_t idx_b = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    const expr* var_b = manifest.expr_pool_.make(idx_b);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a, var_b}));

    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);
    const std::string whnf_a_1 =
        std::get<expr::functor>(manifest.bind_map_.whnf(var_a)->content).name;
    const std::string whnf_b_1 =
        std::get<expr::functor>(manifest.bind_map_.whnf(var_b)->content).name;
    manifest.sim_.tear_down();

    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);
    const std::string whnf_a_2 =
        std::get<expr::functor>(manifest.bind_map_.whnf(var_a)->content).name;
    const std::string whnf_b_2 =
        std::get<expr::functor>(manifest.bind_map_.whnf(var_b)->content).name;
    EXPECT_EQ(whnf_a_2, whnf_a_1);
    EXPECT_EQ(whnf_b_2, whnf_b_1);
    manifest.sim_.tear_down();
}

TEST_F(BasicManifestIntegrationTest, SimLifecycleExprPoolInFrameGrowthRevertsOnTearDown) {
    /*
     * Intent: expr_pool growth during a sim frame is reverted by tear_down.
     * initial goals: make_list(suc^5(zero), abc, R).  (added after set_up in test body)
     * rules:
     *   0: make_list(zero, _, nil).
     *   1: make_list(suc(L), A, cons(A, T)) :- make_list(L, A, T).
     */
    expr rule_ignored{expr::var{0}};
    expr rule_l{expr::var{0}};
    expr rule_a{expr::var{1}};
    expr rule_t{expr::var{2}};
    expr zero{expr::functor{"zero", {}}};
    expr nil{expr::functor{"nil", {}}};
    expr head0{expr::functor{"make_list", {&zero, &rule_ignored, &nil}}};
    expr suc_l{expr::functor{"suc", {&rule_l}}};
    expr cons_at{expr::functor{"cons", {&rule_a, &rule_t}}};
    expr head1{expr::functor{"make_list", {&suc_l, &rule_a, &cons_at}}};
    expr body1{expr::functor{"make_list", {&rule_l, &rule_a, &rule_t}}};
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {&body1}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    const size_t expr_before = manifest.expr_pool_.size();

    manifest.sim_.set_up();
    const expr* zero_pool = manifest.expr_pool_.make("zero", {});
    const expr* len = zero_pool;
    for (int i = 0; i < 5; ++i)
        len = manifest.expr_pool_.make("suc", {len});
    const expr* abc = manifest.expr_pool_.make("abc", {});
    const expr* var_r = manifest.expr_pool_.make(manifest.var_sequencer_.next());
    initial_goals.push(manifest.expr_pool_.make("make_list", {len, abc, var_r}));

    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);
    EXPECT_GT(manifest.expr_pool_.size(), expr_before);
    manifest.sim_.tear_down();
    EXPECT_EQ(manifest.expr_pool_.size(), expr_before);
}

TEST_F(BasicManifestIntegrationTest, SimMhuBindsThroughManifest) {
    /*
     * Intent: a solved sim run binds goal vars via MHU/unification through the manifest.
     * initial goals: f(A, B).
     * rules:
     *   0: f(abc, 123).
     */
    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr head{expr::functor{"f", {&abc, &_123}}};
    database.push(rule{&head, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const uint32_t idx_b = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    const expr* var_b = manifest.expr_pool_.make(idx_b);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a, var_b}));

    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);
    const expr::functor& whnf_a =
        std::get<expr::functor>(manifest.bind_map_.whnf(var_a)->content);
    const expr::functor& whnf_b =
        std::get<expr::functor>(manifest.bind_map_.whnf(var_b)->content);
    EXPECT_EQ(whnf_a.name, "abc");
    EXPECT_EQ(whnf_b.name, "123");
    manifest.sim_.tear_down();
}

TEST_F(BasicManifestIntegrationTest, SimMhuDeactivationRecordedInMemory) {
    /*
     * Intent: choosing one of two incompatible ground heads deactivates the other in memory.
     * initial goals: f(A, B).  (added after set_up in test body)
     * rules:
     *   0: f(abc, 123).
     *   1: f(def, 123).
     */
    expr abc{expr::functor{"abc", {}}};
    expr def{expr::functor{"def", {}}};
    expr _123{expr::functor{"123", {}}};
    expr head0{expr::functor{"f", {&abc, &_123}}};
    expr head1{expr::functor{"f", {&def, &_123}}};
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    const goal_lineage* gl = manifest.make_initial_goal_lineage_.make(0);
    const resolution_lineage* rl0 =
        manifest.lineage_pool_.make_resolution_lineage(gl, rule_id{0});
    const resolution_lineage* rl1 =
        manifest.lineage_pool_.make_resolution_lineage(gl, rule_id{1});

    manifest.sim_.set_up();
    const expr* var_a = manifest.expr_pool_.make(manifest.var_sequencer_.next());
    const expr* var_b = manifest.expr_pool_.make(manifest.var_sequencer_.next());
    initial_goals.push(manifest.expr_pool_.make("f", {var_a, var_b}));

    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);
    EXPECT_EQ(manifest.decision_memory_.count(), 1u);
    const lemma dl = manifest.decision_memory_.derive();
    ASSERT_EQ(dl.get_resolutions().size(), 1u);
    const resolution_lineage* chosen = *dl.get_resolutions().begin();
    const resolution_lineage* deactivated =
        (chosen->idx == 0) ? rl1 : rl0;
    EXPECT_TRUE(manifest.deactivated_candidate_memory_.contains(deactivated));
    manifest.sim_.tear_down();
}

TEST_F(BasicManifestIntegrationTest, SimRandomDecisionGeneratorPicksBranchWithFixedSeed) {
    /*
     * Intent: real random_decision_generator records exactly one f-branch decision at seed 42.
     * initial goals: f.
     * rules:
     *   0: f.
     *   1: f.
     */
    expr goal{expr::functor{"f", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);
    EXPECT_EQ(manifest.decision_memory_.count(), 1u);
    const lemma dl = manifest.decision_memory_.derive();
    ASSERT_EQ(dl.get_resolutions().size(), 1u);
    const rule_id chosen = (*dl.get_resolutions().begin())->idx;
    EXPECT_TRUE(chosen == 0 || chosen == 1);
    manifest.sim_.tear_down();
}

// Tier X — single solver tick (cross-component interop)

TEST_F(BasicManifestIntegrationTest, BindingsBeforeTearDown) {
    /*
     * Intent: normalized bindings are readable before tear_down; bindings revert on resume.
     * initial goals: f(A, B).
     * rules:
     *   0: f(abc, 123).
     */
    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr head{expr::functor{"f", {&abc, &_123}}};
    database.push(rule{&head, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const uint32_t idx_b = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    const expr* var_b = manifest.expr_pool_.make(idx_b);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a, var_b}));

    const expr* abc_saved = saved_expr_pool_.import(&abc);
    const expr* _123_saved = saved_expr_pool_.import(&_123);

    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    ASSERT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_EQ(*saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_a))),
        *abc_saved);
    EXPECT_EQ(*saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_b))),
        *_123_saved);

    sm.resume();
    EXPECT_TRUE(std::holds_alternative<expr::var>(
        manifest.bind_map_.whnf(manifest.expr_pool_.make(idx_a))->content));
    EXPECT_TRUE(std::holds_alternative<expr::var>(
        manifest.bind_map_.whnf(manifest.expr_pool_.make(idx_b))->content));
}

TEST_F(BasicManifestIntegrationTest, TickCdclAvoidancesPersistAcrossTearDown) {
    /*
     * Intent: pre-learned CDCL avoidances survive solver tear_down across two solved ticks.
     * initial goals: f.  g.
     * rules:
     *   0: f.   1: f.
     *   2: g.   3: g.
     * setup: learn {f/0, g/2}; pin lineages for f/0, g/2, g/3.
     */
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    expr g_head2{expr::functor{"g", {}}};
    expr g_head3{expr::functor{"g", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head2, {}});
    database.push(rule{&g_head3, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    const goal_lineage* gl0 = manifest.make_initial_goal_lineage_.make(0);
    const goal_lineage* gl1 = manifest.make_initial_goal_lineage_.make(1);
    const resolution_lineage* rl_g0_0 =
        manifest.lineage_pool_.make_resolution_lineage(gl0, rule_id{0});
    const resolution_lineage* rl_g1_2 =
        manifest.lineage_pool_.make_resolution_lineage(gl1, rule_id{2});
    const resolution_lineage* rl_g1_3 =
        manifest.lineage_pool_.make_resolution_lineage(gl1, rule_id{3});

    manifest.cdcl_.learn(lemma{{rl_g0_0, rl_g1_2}});
    manifest.lineage_pool_.pin(rl_g0_0);
    manifest.lineage_pool_.pin(rl_g1_2);
    manifest.lineage_pool_.pin(rl_g1_3);

    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    ASSERT_EQ(sm.consume_yield(), sim_termination::solved);

    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    ASSERT_EQ(sm.consume_yield(), sim_termination::solved);
}

TEST_F(BasicManifestIntegrationTest, TickBacklogsEliminationForInactiveGoal) {
    /*
     * Intent: backlogged elimination on rule 0 yields only rule 1 in the resolution snapshot.
     * initial goals: f.
     * rules:
     *   0: f.   1: f.
     * setup: backlog elimination for f/0 before the tick.
     */
    expr goal{expr::functor{"f", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    const goal_lineage* gl = manifest.make_initial_goal_lineage_.make(0);
    const resolution_lineage* rl0 =
        manifest.lineage_pool_.make_resolution_lineage(gl, rule_id{0});
    const resolution_lineage* rl1 =
        manifest.lineage_pool_.make_resolution_lineage(gl, rule_id{1});
    manifest.elimination_backlog_.insert_backlogged_elimination(rl0);

    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    ASSERT_EQ(sm.consume_yield(), sim_termination::solved);
    const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
    std::vector<rule_id> resolution_ids;
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
        resolution_ids.push_back(rl->idx);
    EXPECT_THAT(resolution_ids, UnorderedElementsAre(rule_id{1}));
}

TEST_F(BasicManifestIntegrationTest, TickDecisionLemmaLineagesPinnedBeforeTrim) {
    /*
     * Intent: decision/resolution lineages from a tick appear in the snapshot (pinned pre-trim).
     * initial goals: f.  g.
     * rules:
     *   0: f.   1: f.
     *   2: g.   3: g.
     * setup: learn {f/0, g/2}; pin f/0, g/2, g/3; expect resolution rules {0, 3}.
     */
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    expr g_head2{expr::functor{"g", {}}};
    expr g_head3{expr::functor{"g", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head2, {}});
    database.push(rule{&g_head3, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    const goal_lineage* gl0 = manifest.make_initial_goal_lineage_.make(0);
    const goal_lineage* gl1 = manifest.make_initial_goal_lineage_.make(1);
    const resolution_lineage* rl_g0_0 =
        manifest.lineage_pool_.make_resolution_lineage(gl0, rule_id{0});
    const resolution_lineage* rl_g1_2 =
        manifest.lineage_pool_.make_resolution_lineage(gl1, rule_id{2});
    const resolution_lineage* rl_g1_3 =
        manifest.lineage_pool_.make_resolution_lineage(gl1, rule_id{3});

    manifest.cdcl_.learn(lemma{{rl_g0_0, rl_g1_2}});
    manifest.lineage_pool_.pin(rl_g0_0);
    manifest.lineage_pool_.pin(rl_g1_2);
    manifest.lineage_pool_.pin(rl_g1_3);

    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    ASSERT_EQ(sm.consume_yield(), sim_termination::solved);
    const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
    std::vector<rule_id> resolution_ids;
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
        resolution_ids.push_back(rl->idx);
    EXPECT_THAT(resolution_ids, UnorderedElementsAre(rule_id{0}, rule_id{3}));
}

TEST_F(BasicManifestIntegrationTest, TickSecondBranchDiffersOnDuplicateRuleProblem) {
    /*
     * Intent: two solver ticks enumerate distinct f branches with distinct resolution keys.
     * initial goals: f.
     * rules:
     *   0: f.   1: f.
     */
    expr goal{expr::functor{"f", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});

    static const std::set<rule_id> kBranches{rule_id{0}, rule_id{1}};

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    ASSERT_EQ(sm.consume_yield(), sim_termination::solved);
    lemma resolution_lemma1 = manifest.resolution_memory_.derive_resolution_lemma();
    for (const resolution_lineage* rl : resolution_lemma1.get_resolutions())
        manifest.lineage_pool_.pin(rl);
    lemma decision_lemma1 = manifest.decision_memory_.derive();
    std::set<rule_id> first_branches;
    for (const resolution_lineage* rl : decision_lemma1.get_resolutions())
        if (kBranches.count(rl->idx))
            first_branches.insert(rl->idx);
    for (const resolution_lineage* rl : resolution_lemma1.get_resolutions())
        if (kBranches.count(rl->idx))
            first_branches.insert(rl->idx);
    ASSERT_EQ(first_branches.size(), 1u);
    EXPECT_EQ(decision_lemma1.get_resolutions().size(), 1u);

    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    ASSERT_EQ(sm.consume_yield(), sim_termination::solved);
    lemma resolution_lemma2 = manifest.resolution_memory_.derive_resolution_lemma();
    for (const resolution_lineage* rl : resolution_lemma2.get_resolutions())
        manifest.lineage_pool_.pin(rl);
    lemma decision_lemma2 = manifest.decision_memory_.derive();
    EXPECT_TRUE(decision_lemma2.get_resolutions().empty());
    std::set<rule_id> second_branches;
    for (const resolution_lineage* rl : decision_lemma2.get_resolutions())
        if (kBranches.count(rl->idx))
            second_branches.insert(rl->idx);
    for (const resolution_lineage* rl : resolution_lemma2.get_resolutions())
        if (kBranches.count(rl->idx))
            second_branches.insert(rl->idx);
    ASSERT_EQ(second_branches.size(), 1u);
    EXPECT_NE(*first_branches.begin(), *second_branches.begin());
    EXPECT_FALSE(decision_lemma1.get_resolutions() == decision_lemma2.get_resolutions()
        && resolution_lemma1.get_resolutions() == resolution_lemma2.get_resolutions());
}

// Tier S — multi-cycle solver (enumeration)

TEST_F(BasicManifestIntegrationTest, SolverVacuousSolvedOnEmptyProblem) {
    /*
     * Intent: solver yields one vacuous solved tick on an empty problem and completes.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_TRUE(manifest.decision_memory_.derive().get_resolutions().empty());
    sm.resume();
    EXPECT_FALSE(sm.has_yield());
}

TEST_F(BasicManifestIntegrationTest, SolverFindsSingleUnitSolution) {
    /*
     * Intent: a single ground rule solves f without any decision.
     * initial goals: f.
     * rules:
     *   0: f.
     */
    expr goal{expr::functor{"f", {}}};
    expr head{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&head, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_TRUE(manifest.decision_memory_.derive().get_resolutions().empty());
    sm.resume();
    EXPECT_FALSE(sm.has_yield());
}

TEST_F(BasicManifestIntegrationTest, SolverRefutesWhenNoCandidates) {
    /*
     * Intent: solver refutes immediately when the goal has no matching rules.
     * initial goals: f.
     * rules: (none)
     */
    expr goal{expr::functor{"f", {}}};
    initial_goals.push(&goal);

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::conflicted);
    sm.resume();
    EXPECT_FALSE(sm.has_yield());
}

TEST_F(BasicManifestIntegrationTest, SolverEnumeratesTwoGroundChoiceSolutions) {
    /*
     * Intent: solver enumerates both duplicate ground heads for f (enumeration only).
     * initial goals: f.
     * rules:
     *   0: f.   1: f.
     */
    expr goal{expr::functor{"f", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    auto sm = manifest.solver_.solve();
    std::set<rule_id> seen;
    size_t solved_count = 0;
    while (true) {
        sm.resume();
        if (!sm.has_yield())
            break;
        if (sm.consume_yield() != sim_termination::solved)
            continue;
        ++solved_count;
        const lemma decision_lemma = manifest.decision_memory_.derive();
        const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
        std::set<rule_id> branches;
        for (const resolution_lineage* rl : decision_lemma.get_resolutions())
            if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                branches.insert(rl->idx);
        for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
            if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                branches.insert(rl->idx);
        ASSERT_EQ(branches.size(), 1u);
        seen.insert(*branches.begin());
    }
    ASSERT_EQ(solved_count, 2u);
    EXPECT_THAT(seen, UnorderedElementsAre(rule_id{0}, rule_id{1}));
}

TEST_F(BasicManifestIntegrationTest, SolverRefutesAfterEnumeratingAllGroundBranches) {
    /*
     * Intent: after both f branches are found, solver completes with two solved ticks.
     * initial goals: f.
     * rules:
     *   0: f.   1: f.
     */
    expr goal{expr::functor{"f", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    auto sm = manifest.solver_.solve();
    std::set<rule_id> seen;
    std::set<rule_id> visited;
    while (seen.size() < 2u) {
        rule_id branch;
        do {
            sm.resume();
            ASSERT_TRUE(sm.has_yield());
            if (sm.consume_yield() != sim_termination::solved)
                continue;
            const lemma decision_lemma = manifest.decision_memory_.derive();
            const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
            std::set<rule_id> branches;
            for (const resolution_lineage* rl : decision_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    branches.insert(rl->idx);
            for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    branches.insert(rl->idx);
            ASSERT_EQ(branches.size(), 1u);
            branch = *branches.begin();
        } while (visited.count(branch));
        visited.insert(branch);
        seen.insert(branch);
    }
    EXPECT_THAT(seen, UnorderedElementsAre(rule_id{0}, rule_id{1}));
    while (true) {
        sm.resume();
        if (!sm.has_yield())
            break;
        if (sm.consume_yield() == sim_termination::solved)
            FAIL() << "unexpected extra solution after exhausting search space";
    }
}

TEST_F(BasicManifestIntegrationTest, SolverFindsClauseDerivedUnitSolution) {
    /*
     * Intent: f :- g(X). with one g/1 fact solves without a decision.
     * initial goals: f.
     * rules:
     *   0: f :- g(X).
     *   1: g(c).
     */
    expr goal{expr::functor{"f", {}}};
    expr rule_var{expr::var{0}};
    expr g_ground{expr::functor{"c", {}}};
    expr g_fact{expr::functor{"g", {&g_ground}}};
    expr f_head{expr::functor{"f", {}}};
    expr g_body{expr::functor{"g", {&rule_var}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head, {&g_body}});
    database.push(rule{&g_fact, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_TRUE(manifest.decision_memory_.derive().get_resolutions().empty());
    sm.resume();
    EXPECT_FALSE(sm.has_yield());
}

TEST_F(BasicManifestIntegrationTest, SolverEnumeratesTwoChoiceClauseSolutions) {
    /*
     * Intent: solver enumerates both g/1 facts for f :- g(X).
     * initial goals: f.
     * rules:
     *   0: f :- g(X).
     *   1: g(abc).   2: g(xyz).
     */
    expr goal{expr::functor{"f", {}}};
    expr rule_var{expr::var{0}};
    expr abc{expr::functor{"abc", {}}};
    expr xyz{expr::functor{"xyz", {}}};
    expr g_fact0{expr::functor{"g", {&abc}}};
    expr g_fact1{expr::functor{"g", {&xyz}}};
    expr f_head{expr::functor{"f", {}}};
    expr g_body{expr::functor{"g", {&rule_var}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head, {&g_body}});
    database.push(rule{&g_fact0, {}});
    database.push(rule{&g_fact1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    auto sm = manifest.solver_.solve();
    std::set<rule_id> seen;
    size_t solved_count = 0;
    while (true) {
        sm.resume();
        if (!sm.has_yield())
            break;
        if (sm.consume_yield() != sim_termination::solved)
            continue;
        ++solved_count;
        const lemma decision_lemma = manifest.decision_memory_.derive();
        const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
        std::set<rule_id> branches;
        for (const resolution_lineage* rl : decision_lemma.get_resolutions())
            if (rl->idx == rule_id{1} || rl->idx == rule_id{2})
                branches.insert(rl->idx);
        for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
            if (rl->idx == rule_id{1} || rl->idx == rule_id{2})
                branches.insert(rl->idx);
        ASSERT_EQ(branches.size(), 1u);
        seen.insert(*branches.begin());
    }
    ASSERT_EQ(solved_count, 2u);
    EXPECT_THAT(seen, UnorderedElementsAre(rule_id{1}, rule_id{2}));
}

TEST_F(BasicManifestIntegrationTest, SolverFindsSolutionWithCorrectBindings) {
    /*
     * Intent: single-tick solver records correct WHNF bindings for both goal vars.
     * initial goals: f(A, B).
     * rules:
     *   0: f(abc, 123).
     */
    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr head{expr::functor{"f", {&abc, &_123}}};
    database.push(rule{&head, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const uint32_t idx_b = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    const expr* var_b = manifest.expr_pool_.make(idx_b);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a, var_b}));

    const expr* abc_saved = saved_expr_pool_.import(&abc);
    const expr* _123_saved = saved_expr_pool_.import(&_123);
    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_TRUE(manifest.decision_memory_.derive().get_resolutions().empty());
    EXPECT_EQ(*saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_a))),
        *abc_saved);
    EXPECT_EQ(*saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_b))),
        *_123_saved);
    sm.resume();
    EXPECT_FALSE(sm.has_yield());
}

TEST_F(BasicManifestIntegrationTest, SolverFindsClauseBodyBindingSolution) {
    /*
     * Intent: clause bodies g(A), h(B) bind both vars via two unit facts, no decision.
     * initial goals: f(A, B).
     * rules:
     *   0: f(A, B) :- g(A), h(B).
     *   1: g(abc).
     *   2: h(123).
     */
    expr rule_var_a{expr::var{0}};
    expr rule_var_b{expr::var{1}};
    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr f_head{expr::functor{"f", {&rule_var_a, &rule_var_b}}};
    expr g_body{expr::functor{"g", {&rule_var_a}}};
    expr h_body{expr::functor{"h", {&rule_var_b}}};
    expr g_head{expr::functor{"g", {&abc}}};
    expr h_head{expr::functor{"h", {&_123}}};
    database.push(rule{&f_head, {&g_body, &h_body}});
    database.push(rule{&g_head, {}});
    database.push(rule{&h_head, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const uint32_t idx_b = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    const expr* var_b = manifest.expr_pool_.make(idx_b);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a, var_b}));

    const expr* abc_saved = saved_expr_pool_.import(&abc);
    const expr* _123_saved = saved_expr_pool_.import(&_123);
    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_EQ(*saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_a))),
        *abc_saved);
    EXPECT_EQ(*saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_b))),
        *_123_saved);
    sm.resume();
    EXPECT_FALSE(sm.has_yield());
}

TEST_F(BasicManifestIntegrationTest, SolverEnumeratesTwoVarChoiceSolutions) {
    /*
     * Intent: solver enumerates f(abc) and f(xyz) bindings for f(A).
     * initial goals: f(A).
     * rules:
     *   0: f(abc).   1: f(xyz).
     */
    expr abc{expr::functor{"abc", {}}};
    expr xyz{expr::functor{"xyz", {}}};
    expr head0{expr::functor{"f", {&abc}}};
    expr head1{expr::functor{"f", {&xyz}}};
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a}));

    const expr* abc_saved = saved_expr_pool_.import(&abc);
    const expr* xyz_saved = saved_expr_pool_.import(&xyz);
    enumerate_all_solutions(
        manifest.solver_,
        {{abc_saved}, {xyz_saved}},
        [&]() -> solution {
            return {saved_expr_pool_.import(
                normalizer.normalize(manifest.expr_pool_.make(idx_a)))};
        });
}

TEST_F(BasicManifestIntegrationTest, SolverRefutesAfterEnumeratingAllVarBranches) {
    /*
     * Intent: both var-binding branches are found, then solver completes (two solved ticks).
     * initial goals: f(A).
     * rules:
     *   0: f(abc).   1: f(xyz).
     */
    expr abc{expr::functor{"abc", {}}};
    expr xyz{expr::functor{"xyz", {}}};
    expr head0{expr::functor{"f", {&abc}}};
    expr head1{expr::functor{"f", {&xyz}}};
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a}));

    const expr* abc_saved = saved_expr_pool_.import(&abc);
    const expr* xyz_saved = saved_expr_pool_.import(&xyz);
    next_until_refuted(
        manifest.solver_,
        {{abc_saved}, {xyz_saved}},
        [&]() -> solution {
            return {saved_expr_pool_.import(
                normalizer.normalize(manifest.expr_pool_.make(idx_a)))};
        });
}

TEST_F(BasicManifestIntegrationTest, SolverEnumeratesTwoGoalSharedVarSolutions) {
    /*
     * Intent: shared var A on f(A) and g(A) collapses to two binding solutions (abc, xyz).
     * initial goals: f(A).  g(A).
     * rules:
     *   0: f(abc).   1: f(xyz).
     *   2: g(abc).   3: g(xyz).
     */
    expr abc{expr::functor{"abc", {}}};
    expr xyz{expr::functor{"xyz", {}}};
    expr f_head0{expr::functor{"f", {&abc}}};
    expr f_head1{expr::functor{"f", {&xyz}}};
    expr g_head0{expr::functor{"g", {&abc}}};
    expr g_head1{expr::functor{"g", {&xyz}}};
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head0, {}});
    database.push(rule{&g_head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a}));
    initial_goals.push(manifest.expr_pool_.make("g", {var_a}));

    const expr* abc_saved = saved_expr_pool_.import(&abc);
    const expr* xyz_saved = saved_expr_pool_.import(&xyz);
    enumerate_all_solutions(
        manifest.solver_,
        {{abc_saved}, {xyz_saved}},
        [&]() -> solution {
            return {saved_expr_pool_.import(
                normalizer.normalize(manifest.expr_pool_.make(idx_a)))};
        });
}

// Cross-tick enumeration end-to-end.

TEST_F(BasicManifestIntegrationTest, TickThreeGroundBranchesEnumerateDistinct) {
    /*
     * Intent: cross-tick solver enumerates three duplicate f ground branches end-to-end.
     * initial goals: f.
     * rules:
     *   0: f.   1: f.   2: f.
     */
    expr goal{expr::functor{"f", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    expr f_head2{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&f_head2, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    auto sm = manifest.solver_.solve();
    std::set<rule_id> seen;
    std::set<rule_id> visited;
    while (seen.size() < 3u) {
        rule_id branch;
        do {
            sm.resume();
            ASSERT_TRUE(sm.has_yield());
            if (sm.consume_yield() != sim_termination::solved)
                continue;
            const lemma decision_lemma = manifest.decision_memory_.derive();
            const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
            std::set<rule_id> branches;
            for (const resolution_lineage* rl : decision_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1} || rl->idx == rule_id{2})
                    branches.insert(rl->idx);
            for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1} || rl->idx == rule_id{2})
                    branches.insert(rl->idx);
            ASSERT_EQ(branches.size(), 1u);
            branch = *branches.begin();
        } while (visited.count(branch));
        visited.insert(branch);
        seen.insert(branch);
    }
    EXPECT_THAT(seen, UnorderedElementsAre(rule_id{0}, rule_id{1}, rule_id{2}));
    while (true) {
        sm.resume();
        if (!sm.has_yield())
            break;
        if (sm.consume_yield() == sim_termination::solved)
            FAIL() << "unexpected extra solution after exhausting search space";
    }
}

TEST_F(BasicManifestIntegrationTest, SimLifecycleTwoSequentialDecisionsOnTwoGoals) {
    /*
     * Intent: sim records two decisions (one f-branch, one g-branch) on a two-goal problem.
     * initial goals: f.  g.
     * rules:
     *   0: f.   1: f.
     *   2: g.   3: g.
     */
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    expr g_head2{expr::functor{"g", {}}};
    expr g_head3{expr::functor{"g", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head2, {}});
    database.push(rule{&g_head3, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);
    EXPECT_EQ(manifest.decision_memory_.count(), 2u);

    const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
    ASSERT_EQ(resolution_lemma.get_resolutions().size(), 2u);

    std::set<rule_id> f_branches;
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
        if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
            f_branches.insert(rl->idx);
    std::set<rule_id> g_branches;
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
        if (rl->idx == rule_id{2} || rl->idx == rule_id{3})
            g_branches.insert(rl->idx);
    ASSERT_EQ(f_branches.size(), 1u);
    ASSERT_EQ(g_branches.size(), 1u);
    manifest.sim_.tear_down();
}

TEST_F(BasicManifestIntegrationTest, SimLifecycleRecursiveClauseTreeSolvedWithoutDecisions) {
    /*
     * Intent: recursive clause tree solves f with zero recorded decisions.
     * initial goals: f.
     * rules:
     *   0: f :- g, h.
     *   1: g :- i, j.
     *   2: h :- i, j.
     *   3: i.
     *   4: j.
     */
    expr goal_f{expr::functor{"f", {}}};
    expr f_head{expr::functor{"f", {}}};
    expr g_body{expr::functor{"g", {}}};
    expr h_body{expr::functor{"h", {}}};
    expr g_head{expr::functor{"g", {}}};
    expr h_head{expr::functor{"h", {}}};
    expr i_body{expr::functor{"i", {}}};
    expr j_body{expr::functor{"j", {}}};
    expr i_head{expr::functor{"i", {}}};
    expr j_head{expr::functor{"j", {}}};
    initial_goals.push(&goal_f);
    database.push(rule{&f_head, {&g_body, &h_body}});
    database.push(rule{&g_head, {&i_body, &j_body}});
    database.push(rule{&h_head, {&i_body, &j_body}});
    database.push(rule{&i_head, {}});
    database.push(rule{&j_head, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);
    EXPECT_EQ(manifest.decision_memory_.count(), 0u);
    manifest.sim_.tear_down();
}

TEST_F(BasicManifestIntegrationTest, SimLifecycleCdclUnitElimForcesRemainingCandidate) {
    /*
     * Intent: CDCL unit-elim on g/2 after f commits; seed-independent branch invariants.
     * initial goals: f.  g.
     * rules:
     *   0: f.   1: f.
     *   2: g.   3: g.
     * setup: learn {f/0, g/2} and {f/1, g/2}.
     */
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    expr g_head0{expr::functor{"g", {}}};
    expr g_head1{expr::functor{"g", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head0, {}});
    database.push(rule{&g_head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    const goal_lineage* gl_f = manifest.make_initial_goal_lineage_.make(0);
    const goal_lineage* gl_g = manifest.make_initial_goal_lineage_.make(1);
    const resolution_lineage* rl_f_0 =
        manifest.lineage_pool_.make_resolution_lineage(gl_f, rule_id{0});
    const resolution_lineage* rl_f_1 =
        manifest.lineage_pool_.make_resolution_lineage(gl_f, rule_id{1});
    const resolution_lineage* rl_g_2 =
        manifest.lineage_pool_.make_resolution_lineage(gl_g, rule_id{2});

    manifest.cdcl_.learn(lemma{{rl_f_0, rl_g_2}});
    manifest.cdcl_.learn(lemma{{rl_f_1, rl_g_2}});

    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);

    const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
    ASSERT_EQ(resolution_lemma.get_resolutions().size(), 2u);

    std::set<rule_id> f_branches;
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
        if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
            f_branches.insert(rl->idx);
    std::set<rule_id> g_branches;
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
        if (rl->idx == rule_id{2} || rl->idx == rule_id{3})
            g_branches.insert(rl->idx);
    ASSERT_EQ(f_branches.size(), 1u);
    ASSERT_EQ(g_branches.size(), 1u);
    // CDCL forbids g/2 once f is committed; when RNG picks f before g (one decision),
    // g/3 is unit-forced. Two decisions occur if g is chosen first — still valid.
    if (manifest.decision_memory_.count() == 1u)
        EXPECT_FALSE(g_branches.contains(rule_id{2}));
    manifest.sim_.tear_down();
}

TEST_F(BasicManifestIntegrationTest, SimLifecycleDecisionMemoryClearsEachTearDown) {
    /*
     * Intent: decision_memory_ is empty after each tear_down across two non-vacuous sim cycles.
     * initial goals: f.  g.
     * rules:
     *   0: f.   1: f.
     *   2: g.   3: g.
     * setup: learn {f/0, g/2}; pin f/0, g/2, g/3.
     */
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    expr g_head2{expr::functor{"g", {}}};
    expr g_head3{expr::functor{"g", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head2, {}});
    database.push(rule{&g_head3, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    const goal_lineage* gl0 = manifest.make_initial_goal_lineage_.make(0);
    const goal_lineage* gl1 = manifest.make_initial_goal_lineage_.make(1);
    const resolution_lineage* rl_g0_0 =
        manifest.lineage_pool_.make_resolution_lineage(gl0, rule_id{0});
    const resolution_lineage* rl_g1_2 =
        manifest.lineage_pool_.make_resolution_lineage(gl1, rule_id{2});
    const resolution_lineage* rl_g1_3 =
        manifest.lineage_pool_.make_resolution_lineage(gl1, rule_id{3});

    manifest.cdcl_.learn(lemma{{rl_g0_0, rl_g1_2}});
    manifest.lineage_pool_.pin(rl_g0_0);
    manifest.lineage_pool_.pin(rl_g1_2);
    manifest.lineage_pool_.pin(rl_g1_3);

    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);
    manifest.sim_.tear_down();
    EXPECT_EQ(manifest.decision_memory_.count(), 0u);

    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);
    manifest.sim_.tear_down();
    EXPECT_EQ(manifest.decision_memory_.count(), 0u);
}

TEST_F(BasicManifestIntegrationTest, SolverEnumeratesFourTwoGoalGroundCombinations) {
    /*
     * Intent: solver enumerates all 2×2 ground branch pairs for independent f and g goals.
     * initial goals: f.  g.
     * rules:
     *   0: f.   1: f.
     *   2: g.   3: g.
     */
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    expr g_head2{expr::functor{"g", {}}};
    expr g_head3{expr::functor{"g", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head2, {}});
    database.push(rule{&g_head3, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    auto sm = manifest.solver_.solve();
    std::set<std::pair<rule_id, rule_id>> seen;
    std::set<std::pair<rule_id, rule_id>> visited;
    while (seen.size() < 4u) {
        std::pair<rule_id, rule_id> pair;
        do {
            sm.resume();
            ASSERT_TRUE(sm.has_yield());
            if (sm.consume_yield() != sim_termination::solved)
                continue;
            const lemma decision_lemma = manifest.decision_memory_.derive();
            const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
            std::set<rule_id> f_branch;
            for (const resolution_lineage* rl : decision_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    f_branch.insert(rl->idx);
            for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    f_branch.insert(rl->idx);
            std::set<rule_id> g_branch;
            for (const resolution_lineage* rl : decision_lemma.get_resolutions())
                if (rl->idx == rule_id{2} || rl->idx == rule_id{3})
                    g_branch.insert(rl->idx);
            for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{2} || rl->idx == rule_id{3})
                    g_branch.insert(rl->idx);
            ASSERT_EQ(f_branch.size(), 1u);
            ASSERT_EQ(g_branch.size(), 1u);
            pair = {*f_branch.begin(), *g_branch.begin()};
        } while (visited.count(pair));
        visited.insert(pair);
        seen.insert(pair);
    }
    EXPECT_TRUE(seen.contains({rule_id{0}, rule_id{2}}));
    EXPECT_TRUE(seen.contains({rule_id{0}, rule_id{3}}));
    EXPECT_TRUE(seen.contains({rule_id{1}, rule_id{2}}));
    EXPECT_TRUE(seen.contains({rule_id{1}, rule_id{3}}));
    while (true) {
        sm.resume();
        if (!sm.has_yield())
            break;
        if (sm.consume_yield() == sim_termination::solved)
            FAIL() << "unexpected extra solution after exhausting search space";
    }
}

TEST_F(BasicManifestIntegrationTest, SolverEnumeratesEightThreeGoalGroundCombinations) {
    /*
     * Intent: seed-independent 2×2×2 enumeration across three duplicate-goal ground problems.
     * initial goals: f.  g.  h.
     * rules:
     *   0: f.   1: f.
     *   2: g.   3: g.
     *   4: h.   5: h.
     */
    static constexpr std::array<uint32_t, 10> kSeeds = {
        0, 1, 7, 13, 42, 99, 123, 256, 1000, 9999};

    for (uint32_t seed : kSeeds) {
        SCOPED_TRACE(testing::Message() << "seed=" << seed);

        db seed_db;
        initial_goal_exprs seed_goals;
        expr goal_f{expr::functor{"f", {}}};
        expr goal_g{expr::functor{"g", {}}};
        expr goal_h{expr::functor{"h", {}}};
        expr f0{expr::functor{"f", {}}};
        expr f1{expr::functor{"f", {}}};
        expr g0{expr::functor{"g", {}}};
        expr g1{expr::functor{"g", {}}};
        expr h0{expr::functor{"h", {}}};
        expr h1{expr::functor{"h", {}}};
        seed_goals.push(&goal_f);
        seed_goals.push(&goal_g);
        seed_goals.push(&goal_h);
        seed_db.push(rule{&f0, {}});
        seed_db.push(rule{&f1, {}});
        seed_db.push(rule{&g0, {}});
        seed_db.push(rule{&g1, {}});
        seed_db.push(rule{&h0, {}});
        seed_db.push(rule{&h1, {}});

        trail seed_trail;
        locator seed_loc;
        seed_loc.bind_as<i_log_to_current_trail_frame>(seed_trail);
        expr_pool seed_pool{seed_loc};
        basic_manifest manifest{seed_db, seed_goals, kMaxResolutions, seed};
        normalizer seed_normalizer{manifest.loc_};

        auto sm = manifest.solver_.solve();
        std::set<std::tuple<rule_id, rule_id, rule_id>> seen;
        size_t solved_count = 0;
        while (true) {
            sm.resume();
            if (!sm.has_yield())
                break;
            if (sm.consume_yield() != sim_termination::solved)
                continue;
            ++solved_count;
            const lemma decision_lemma = manifest.decision_memory_.derive();
            const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
            std::set<rule_id> f_branch;
            for (const resolution_lineage* rl : decision_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    f_branch.insert(rl->idx);
            for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    f_branch.insert(rl->idx);
            std::set<rule_id> g_branch;
            for (const resolution_lineage* rl : decision_lemma.get_resolutions())
                if (rl->idx == rule_id{2} || rl->idx == rule_id{3})
                    g_branch.insert(rl->idx);
            for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{2} || rl->idx == rule_id{3})
                    g_branch.insert(rl->idx);
            std::set<rule_id> h_branch;
            for (const resolution_lineage* rl : decision_lemma.get_resolutions())
                if (rl->idx == rule_id{4} || rl->idx == rule_id{5})
                    h_branch.insert(rl->idx);
            for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{4} || rl->idx == rule_id{5})
                    h_branch.insert(rl->idx);
            ASSERT_EQ(f_branch.size(), 1u);
            ASSERT_EQ(g_branch.size(), 1u);
            ASSERT_EQ(h_branch.size(), 1u);
            seen.insert({*f_branch.begin(), *g_branch.begin(), *h_branch.begin()});
        }
        ASSERT_EQ(solved_count, 8u);
        EXPECT_TRUE(seen.contains({rule_id{0}, rule_id{2}, rule_id{4}}));
        EXPECT_TRUE(seen.contains({rule_id{0}, rule_id{2}, rule_id{5}}));
        EXPECT_TRUE(seen.contains({rule_id{0}, rule_id{3}, rule_id{4}}));
        EXPECT_TRUE(seen.contains({rule_id{0}, rule_id{3}, rule_id{5}}));
        EXPECT_TRUE(seen.contains({rule_id{1}, rule_id{2}, rule_id{4}}));
        EXPECT_TRUE(seen.contains({rule_id{1}, rule_id{2}, rule_id{5}}));
        EXPECT_TRUE(seen.contains({rule_id{1}, rule_id{3}, rule_id{4}}));
        EXPECT_TRUE(seen.contains({rule_id{1}, rule_id{3}, rule_id{5}}));
        EXPECT_EQ(seen.size(), 8u);
    }
}

TEST_F(BasicManifestIntegrationTest, SolverEnumeratesFourVarBindingSolutions) {
    /*
     * Intent: solver enumerates four distinct bindings for f(A) across four ground heads.
     * initial goals: f(A).
     * rules:
     *   0: f(abc).   1: f(xyz).   2: f(def).   3: f(ghi).
     */
    expr abc{expr::functor{"abc", {}}};
    expr xyz{expr::functor{"xyz", {}}};
    expr def{expr::functor{"def", {}}};
    expr ghi{expr::functor{"ghi", {}}};
    expr head0{expr::functor{"f", {&abc}}};
    expr head1{expr::functor{"f", {&xyz}}};
    expr head2{expr::functor{"f", {&def}}};
    expr head3{expr::functor{"f", {&ghi}}};
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {}});
    database.push(rule{&head2, {}});
    database.push(rule{&head3, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a}));

    const expr* abc_saved = saved_expr_pool_.import(&abc);
    const expr* xyz_saved = saved_expr_pool_.import(&xyz);
    const expr* def_saved = saved_expr_pool_.import(&def);
    const expr* ghi_saved = saved_expr_pool_.import(&ghi);
    next_until_refuted(
        manifest.solver_,
        {{abc_saved}, {xyz_saved}, {def_saved}, {ghi_saved}},
        [&]() -> solution {
            return {saved_expr_pool_.import(
                normalizer.normalize(manifest.expr_pool_.make(idx_a)))};
        });
}

TEST_F(BasicManifestIntegrationTest, SolverEnumeratesFourClauseBodyFactChoices) {
    /*
     * Intent: solver enumerates four g/1 facts for f :- g(X).
     * initial goals: f.
     * rules:
     *   0: f :- g(X).
     *   1: g(a).   2: g(b).   3: g(c).   4: g(d).
     */
    expr goal{expr::functor{"f", {}}};
    expr rule_var{expr::var{0}};
    expr a{expr::functor{"a", {}}};
    expr b{expr::functor{"b", {}}};
    expr c{expr::functor{"c", {}}};
    expr d{expr::functor{"d", {}}};
    expr g_fact1{expr::functor{"g", {&a}}};
    expr g_fact2{expr::functor{"g", {&b}}};
    expr g_fact3{expr::functor{"g", {&c}}};
    expr g_fact4{expr::functor{"g", {&d}}};
    expr f_head{expr::functor{"f", {}}};
    expr g_body{expr::functor{"g", {&rule_var}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head, {&g_body}});
    database.push(rule{&g_fact1, {}});
    database.push(rule{&g_fact2, {}});
    database.push(rule{&g_fact3, {}});
    database.push(rule{&g_fact4, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    auto sm = manifest.solver_.solve();
    std::set<rule_id> seen;
    std::set<rule_id> visited;
    while (seen.size() < 4u) {
        rule_id fact;
        do {
            sm.resume();
            ASSERT_TRUE(sm.has_yield());
            if (sm.consume_yield() != sim_termination::solved)
                continue;
            const lemma decision_lemma = manifest.decision_memory_.derive();
            const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
            std::set<rule_id> g_fact;
            for (const resolution_lineage* rl : decision_lemma.get_resolutions())
                if (rl->idx == rule_id{1} || rl->idx == rule_id{2} || rl->idx == rule_id{3}
                    || rl->idx == rule_id{4})
                    g_fact.insert(rl->idx);
            for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{1} || rl->idx == rule_id{2} || rl->idx == rule_id{3}
                    || rl->idx == rule_id{4})
                    g_fact.insert(rl->idx);
            ASSERT_EQ(g_fact.size(), 1u);
            fact = *g_fact.begin();
        } while (visited.count(fact));
        visited.insert(fact);
        seen.insert(fact);
    }
    EXPECT_THAT(seen, UnorderedElementsAre(rule_id{1}, rule_id{2}, rule_id{3}, rule_id{4}));
    while (true) {
        sm.resume();
        if (!sm.has_yield())
            break;
        if (sm.consume_yield() == sim_termination::solved)
            FAIL() << "unexpected extra solution after exhausting search space";
    }
}

TEST_F(BasicManifestIntegrationTest, SolverEnumeratesManySharedVarGroundHeads) {
    /*
     * Intent: enumerate 2×5 (f-branch, g-branch) paths collapsing to five g(A,B,C) binding triples.
     * initial goals: f.  g(A, B, C).
     * rules:
     *   0: f.   1: f.
     *   2: g(abc, xyz, pqr).   3: g(def, xyz, pqr).   4: g(ghi, xyz, pqr).
     *   5: g(jkl, xyz, pqr).   6: g(mno, xyz, pqr).
     * note: sim stress g_bad and pre-learn CDCL are covered in sim.cpp; pre-learn CDCL before
     * multi-tick enumeration currently hits CDCL watched-goal erase failures.
     */
    static constexpr std::array<uint32_t, 10> kSeeds = {
        0, 1, 7, 13, 42, 99, 123, 256, 1000, 9999};
    static constexpr size_t kRawSolutions = 10;  // |f| × |g| = 2 × 5 resolution paths

    for (uint32_t seed : kSeeds) {
        SCOPED_TRACE(testing::Message() << "seed=" << seed);

        db seed_db;
        initial_goal_exprs seed_goals;
        expr abc{expr::functor{"abc", {}}};
        expr def{expr::functor{"def", {}}};
        expr ghi{expr::functor{"ghi", {}}};
        expr jkl{expr::functor{"jkl", {}}};
        expr mno{expr::functor{"mno", {}}};
        expr pqr{expr::functor{"pqr", {}}};
        expr xyz{expr::functor{"xyz", {}}};
        expr f_head0{expr::functor{"f", {}}};
        expr f_head1{expr::functor{"f", {}}};
        expr g_abc{expr::functor{"g", {&abc, &xyz, &pqr}}};
        expr g_def{expr::functor{"g", {&def, &xyz, &pqr}}};
        expr g_ghi{expr::functor{"g", {&ghi, &xyz, &pqr}}};
        expr g_jkl{expr::functor{"g", {&jkl, &xyz, &pqr}}};
        expr g_mno{expr::functor{"g", {&mno, &xyz, &pqr}}};
        seed_db.push(rule{&f_head0, {}});
        seed_db.push(rule{&f_head1, {}});
        seed_db.push(rule{&g_abc, {}});
        seed_db.push(rule{&g_def, {}});
        seed_db.push(rule{&g_ghi, {}});
        seed_db.push(rule{&g_jkl, {}});
        seed_db.push(rule{&g_mno, {}});

        trail seed_trail;
        locator seed_loc;
        seed_loc.bind_as<i_log_to_current_trail_frame>(seed_trail);
        expr_pool seed_pool{seed_loc};
        basic_manifest manifest{seed_db, seed_goals, kMaxResolutions, seed};
        normalizer seed_normalizer{manifest.loc_};

        const uint32_t idx_a = manifest.var_sequencer_.next();
        const uint32_t idx_b = manifest.var_sequencer_.next();
        const uint32_t idx_c = manifest.var_sequencer_.next();
        const expr* var_a = manifest.expr_pool_.make(idx_a);
        const expr* var_b = manifest.expr_pool_.make(idx_b);
        const expr* var_c = manifest.expr_pool_.make(idx_c);
        seed_goals.push(manifest.expr_pool_.make("f", {}));
        seed_goals.push(manifest.expr_pool_.make("g", {var_a, var_b, var_c}));

        auto sm = manifest.solver_.solve();
        std::vector<solution> binding_solutions;
        std::set<std::pair<rule_id, rule_id>> seen;
        while (true) {
            sm.resume();
            if (!sm.has_yield())
                break;
            if (sm.consume_yield() != sim_termination::solved)
                continue;
            binding_solutions.push_back({
                seed_pool.import(seed_normalizer.normalize(var_a)),
                seed_pool.import(seed_normalizer.normalize(var_b)),
                seed_pool.import(seed_normalizer.normalize(var_c)),
            });
            const lemma decision_lemma = manifest.decision_memory_.derive();
            const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
            std::set<rule_id> f_branch;
            for (const resolution_lineage* rl : decision_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    f_branch.insert(rl->idx);
            for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    f_branch.insert(rl->idx);
            std::set<rule_id> g_branch;
            for (const resolution_lineage* rl : decision_lemma.get_resolutions())
                if (rl->idx == rule_id{2} || rl->idx == rule_id{3} || rl->idx == rule_id{4}
                    || rl->idx == rule_id{5} || rl->idx == rule_id{6})
                    g_branch.insert(rl->idx);
            for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{2} || rl->idx == rule_id{3} || rl->idx == rule_id{4}
                    || rl->idx == rule_id{5} || rl->idx == rule_id{6})
                    g_branch.insert(rl->idx);
            ASSERT_EQ(f_branch.size(), 1u);
            ASSERT_EQ(g_branch.size(), 1u);
            seen.insert({*f_branch.begin(), *g_branch.begin()});
        }
        ASSERT_EQ(binding_solutions.size(), kRawSolutions);
        EXPECT_TRUE(seen.contains({rule_id{0}, rule_id{2}}));
        EXPECT_TRUE(seen.contains({rule_id{0}, rule_id{3}}));
        EXPECT_TRUE(seen.contains({rule_id{0}, rule_id{4}}));
        EXPECT_TRUE(seen.contains({rule_id{0}, rule_id{5}}));
        EXPECT_TRUE(seen.contains({rule_id{0}, rule_id{6}}));
        EXPECT_TRUE(seen.contains({rule_id{1}, rule_id{2}}));
        EXPECT_TRUE(seen.contains({rule_id{1}, rule_id{3}}));
        EXPECT_TRUE(seen.contains({rule_id{1}, rule_id{4}}));
        EXPECT_TRUE(seen.contains({rule_id{1}, rule_id{5}}));
        EXPECT_TRUE(seen.contains({rule_id{1}, rule_id{6}}));
        EXPECT_EQ(seen.size(), kRawSolutions);
        const expr* abc_saved = seed_pool.import(&abc);
        const expr* def_saved = seed_pool.import(&def);
        const expr* ghi_saved = seed_pool.import(&ghi);
        const expr* jkl_saved = seed_pool.import(&jkl);
        const expr* mno_saved = seed_pool.import(&mno);
        const expr* xyz_saved = seed_pool.import(&xyz);
        const expr* pqr_saved = seed_pool.import(&pqr);
        size_t saw_abc = 0;
        size_t saw_def = 0;
        size_t saw_ghi = 0;
        size_t saw_jkl = 0;
        size_t saw_mno = 0;
        for (const solution& s : binding_solutions) {
            ASSERT_EQ(s.size(), 3u);
            const expr& a_bound = *s[0];
            if (a_bound == *abc_saved)
                ++saw_abc;
            else if (a_bound == *def_saved)
                ++saw_def;
            else if (a_bound == *ghi_saved)
                ++saw_ghi;
            else if (a_bound == *jkl_saved)
                ++saw_jkl;
            else if (a_bound == *mno_saved)
                ++saw_mno;
            else
                FAIL() << "unexpected binding for idx_a";
            EXPECT_EQ(*s[1], *xyz_saved);
            EXPECT_EQ(*s[2], *pqr_saved);
        }
        EXPECT_EQ(saw_abc, 2u);
        EXPECT_EQ(saw_def, 2u);
        EXPECT_EQ(saw_ghi, 2u);
        EXPECT_EQ(saw_jkl, 2u);
        EXPECT_EQ(saw_mno, 2u);
    }
}

// Tier R — ridge CHC ports (old-main-test.cpp test_ridge() tests 5–19)

TEST_F(BasicManifestIntegrationTest, RidgeRefutesAfterCdclOnUnsatClauseBranches) {
    /*
     * Intent: multi-tick runtime CDCL refutation on unsatisfiable a :- b / a :- c (NOT empty-DB refutation).
     * initial goals: a.
     * rules:
     *   0: a :- b.   1: a :- c.
     */
    expr b{expr::functor{"b", {}}};
    expr c{expr::functor{"c", {}}};
    expr a_head0{expr::functor{"a", {}}};
    expr a_head1{expr::functor{"a", {}}};
    database.push(rule{&a_head0, {&b}});
    database.push(rule{&a_head1, {&c}});

    expr goal{expr::functor{"a", {}}};
    initial_goals.push(&goal);

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    auto sm = manifest.solver_.solve();
    size_t yields = 0;
    bool saw_conflicted = false;
    while (true) {
        sm.resume();
        if (!sm.has_yield())
            break;
        const auto term = sm.consume_yield();
        ++yields;
        if (term == sim_termination::solved)
            FAIL() << "unexpected solved termination";
        if (term == sim_termination::conflicted)
            saw_conflicted = true;
    }
    ASSERT_GE(yields, 2u);
    EXPECT_TRUE(saw_conflicted);
}

TEST_F(BasicManifestIntegrationTest, RidgeFindsUniqueSharedVarConjunctionThenRefutes) {
    /*
     * Intent: is_a(X) ∧ is_b(X) has unique binding X=2, then refutes.
     * initial goals: is_a(X).  is_b(X).
     * rules:
     *   0: is_a(1).   1: is_a(2).   2: is_b(2).   3: is_b(3).
     */
    expr one{expr::functor{"1", {}}};
    expr two{expr::functor{"2", {}}};
    expr three{expr::functor{"3", {}}};
    expr is_a1{expr::functor{"is_a", {&one}}};
    expr is_a2{expr::functor{"is_a", {&two}}};
    expr is_b2{expr::functor{"is_b", {&two}}};
    expr is_b3{expr::functor{"is_b", {&three}}};
    database.push(rule{&is_a1, {}});
    database.push(rule{&is_a2, {}});
    database.push(rule{&is_b2, {}});
    database.push(rule{&is_b3, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_x = manifest.var_sequencer_.next();
    const expr* var_x = manifest.expr_pool_.make(idx_x);
    initial_goals.push(manifest.expr_pool_.make("is_a", {var_x}));
    initial_goals.push(manifest.expr_pool_.make("is_b", {var_x}));

    const expr* two_saved = saved_expr_pool_.import(&two);
    next_until_refuted(
        manifest.solver_,
        {{two_saved}},
        [&]() -> solution {
            return {saved_expr_pool_.import(
                normalizer.normalize(manifest.expr_pool_.make(idx_x)))};
        });
}

TEST_F(BasicManifestIntegrationTest, RidgeEnumeratesTwoParentBindingsForAlice) {
    /*
     * Intent: parent(X, alice) enumerates bob and carol; parent(dave,bob) is head-elim junk.
     * initial goals: parent(X, alice).
     * rules:
     *   0: parent(bob, alice).   1: parent(carol, alice).   2: parent(dave, bob).
     */
    expr bob{expr::functor{"bob", {}}};
    expr carol{expr::functor{"carol", {}}};
    expr alice{expr::functor{"alice", {}}};
    expr dave{expr::functor{"dave", {}}};
    expr parent_bob_alice{expr::functor{"parent", {&bob, &alice}}};
    expr parent_carol_alice{expr::functor{"parent", {&carol, &alice}}};
    expr parent_dave_bob{expr::functor{"parent", {&dave, &bob}}};
    database.push(rule{&parent_bob_alice, {}});
    database.push(rule{&parent_carol_alice, {}});
    database.push(rule{&parent_dave_bob, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_x = manifest.var_sequencer_.next();
    const expr* var_x = manifest.expr_pool_.make(idx_x);
    const expr* alice_goal = manifest.expr_pool_.make("alice", {});
    initial_goals.push(manifest.expr_pool_.make("parent", {var_x, alice_goal}));

    const expr* bob_saved = saved_expr_pool_.import(&bob);
    const expr* carol_saved = saved_expr_pool_.import(&carol);
    next_until_refuted(
        manifest.solver_,
        {{bob_saved}, {carol_saved}},
        [&]() -> solution {
            return {saved_expr_pool_.import(
                normalizer.normalize(manifest.expr_pool_.make(idx_x)))};
        });
}

TEST_F(BasicManifestIntegrationTest, RidgeEnumeratesPeanoLessThanSeven) {
    /*
     * Intent: lt(N, suc^7(zero)) — seven Peano solutions N ∈ {0..6}.
     * Ported from old-main-test.cpp test_ridge() test 14.
     * initial goals: lt(N, seven).
     * rules:
     *   0: nat(zero).   1: nat(suc(X)) :- nat(X).
     *   2: lt(zero, suc(X)) :- nat(X).   3: lt(suc(X), suc(Y)) :- lt(X, Y).
     */
    static constexpr size_t kPeanoBudget = 128;

    expr zero{expr::functor{"zero", {}}};
    expr nat_zero_h{expr::functor{"nat", {&zero}}};
    database.push(rule{&nat_zero_h, {}});

    expr rv1{expr::var{0}};
    expr suc_rv1{expr::functor{"suc", {&rv1}}};
    expr nat_suc_rv1{expr::functor{"nat", {&suc_rv1}}};
    expr nat_rv1{expr::functor{"nat", {&rv1}}};
    database.push(rule{&nat_suc_rv1, {&nat_rv1}});

    expr rv2{expr::var{0}};
    expr suc_rv2{expr::functor{"suc", {&rv2}}};
    expr lt_zero_suc{expr::functor{"lt", {&zero, &suc_rv2}}};
    expr nat_rv2{expr::functor{"nat", {&rv2}}};
    database.push(rule{&lt_zero_suc, {&nat_rv2}});

    expr rv3{expr::var{0}};
    expr rv4{expr::var{1}};
    expr suc_rv3{expr::functor{"suc", {&rv3}}};
    expr suc_rv4{expr::functor{"suc", {&rv4}}};
    expr lt_suc_suc{expr::functor{"lt", {&suc_rv3, &suc_rv4}}};
    expr lt_rv3_rv4{expr::functor{"lt", {&rv3, &rv4}}};
    database.push(rule{&lt_suc_suc, {&lt_rv3_rv4}});

    std::set<solution> expected;
    for (int n = 0; n < 7; ++n) {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        expected.insert({p});
    }

    basic_manifest manifest{database, initial_goals, kPeanoBudget, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_n = manifest.var_sequencer_.next();
    const expr* var_n = manifest.expr_pool_.make(idx_n);
    const expr* seven = manifest.expr_pool_.make("zero", {});
    for (int i = 0; i < 7; ++i)
        seven = manifest.expr_pool_.make("suc", {seven});
    initial_goals.push(manifest.expr_pool_.make("lt", {var_n, seven}));

    auto sm = manifest.solver_.solve();
    std::set<solution> remaining = expected;
    std::set<solution> visited;
    while (!remaining.empty()) {
        sm.resume();
        ASSERT_TRUE(sm.has_yield()) << "solver stopped before all expected solutions found";
        const auto term = sm.consume_yield();
        ASSERT_NE(term, sim_termination::depth_exceeded) << "raise kPeanoBudget";
        if (term != sim_termination::solved)
            continue;
        const solution s = {
            saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_n)))};
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = remaining.find(s);
        ASSERT_NE(it, remaining.end()) << "unexpected solution";
        remaining.erase(it);
    }
}

TEST_F(BasicManifestIntegrationTest, RidgeEnumeratesSatPAndQOrR) {
    /*
     * Intent: P ∧ (Q ∨ R) — three bool assignments (calibration probe for multi-var relational CHC).
     * Ported from old-main-test.cpp test_ridge() test 11.
     * initial goals: bool(P). bool(Q). bool(R). or(Q,R,QR). and(P,QR,true).
     * rules: bool(true/false); relational or/and with bool(X) bodies.
     */
    expr true_atom{expr::functor{"true", {}}};
    expr false_atom{expr::functor{"false", {}}};
    expr bool_true{expr::functor{"bool", {&true_atom}}};
    expr bool_false{expr::functor{"bool", {&false_atom}}};
    database.push(rule{&bool_true, {}});
    database.push(rule{&bool_false, {}});

    expr or_rv{expr::var{0}};
    expr or_true_x_true{expr::functor{"or", {&true_atom, &or_rv, &true_atom}}};
    expr or_bool_body{expr::functor{"bool", {&or_rv}}};
    database.push(rule{&or_true_x_true, {&or_bool_body}});

    expr or_rv2{expr::var{0}};
    expr or_false_x_x{expr::functor{"or", {&false_atom, &or_rv2, &or_rv2}}};
    expr or_bool_body2{expr::functor{"bool", {&or_rv2}}};
    database.push(rule{&or_false_x_x, {&or_bool_body2}});

    expr and_rv{expr::var{0}};
    expr and_true_x_x{expr::functor{"and", {&true_atom, &and_rv, &and_rv}}};
    expr and_bool_body{expr::functor{"bool", {&and_rv}}};
    database.push(rule{&and_true_x_x, {&and_bool_body}});

    expr and_rv2{expr::var{0}};
    expr and_false_x_false{expr::functor{"and", {&false_atom, &and_rv2, &false_atom}}};
    expr and_bool_body2{expr::functor{"bool", {&and_rv2}}};
    database.push(rule{&and_false_x_false, {&and_bool_body2}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_p = manifest.var_sequencer_.next();
    const uint32_t idx_q = manifest.var_sequencer_.next();
    const uint32_t idx_r = manifest.var_sequencer_.next();
    const uint32_t idx_qr = manifest.var_sequencer_.next();
    const expr* var_p = manifest.expr_pool_.make(idx_p);
    const expr* var_q = manifest.expr_pool_.make(idx_q);
    const expr* var_r = manifest.expr_pool_.make(idx_r);
    const expr* var_qr = manifest.expr_pool_.make(idx_qr);
    const expr* true_goal = manifest.expr_pool_.make("true", {});
    initial_goals.push(manifest.expr_pool_.make("bool", {var_p}));
    initial_goals.push(manifest.expr_pool_.make("bool", {var_q}));
    initial_goals.push(manifest.expr_pool_.make("bool", {var_r}));
    initial_goals.push(manifest.expr_pool_.make("or", {var_q, var_r, var_qr}));
    initial_goals.push(manifest.expr_pool_.make("and", {var_p, var_qr, true_goal}));

    const expr* t_saved = saved_expr_pool_.import(&true_atom);
    const expr* f_saved = saved_expr_pool_.import(&false_atom);
    // Raw solved ticks may exceed 3 when resolution paths duplicate bindings.
    next_until_refuted(
        manifest.solver_,
        {{t_saved, t_saved, t_saved}, {t_saved, t_saved, f_saved}, {t_saved, f_saved, t_saved}},
        [&]() -> solution {
            return {
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_p))),
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_q))),
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_r))),
            };
        });
}

TEST_F(BasicManifestIntegrationTest, RidgeEnumeratesTwoSatAssignmentsForImpliesQ) {
    /*
     * Intent: (P∨Q)∧(¬P∨Q) — both solutions have Q=true; P differs.
     * Ported from old-main-test.cpp test_ridge() test 8.
     * initial goals: bool(P). bool(Q). or(P,Q,true). not(P,NP). or(NP,Q,true).
     * rules: 8 bool/or/not ground facts.
     */
    expr true_atom{expr::functor{"true", {}}};
    expr false_atom{expr::functor{"false", {}}};
    expr bool_true{expr::functor{"bool", {&true_atom}}};
    expr bool_false{expr::functor{"bool", {&false_atom}}};
    expr not_true_false{expr::functor{"not", {&true_atom, &false_atom}}};
    expr not_false_true{expr::functor{"not", {&false_atom, &true_atom}}};
    expr or_t_t_t{expr::functor{"or", {&true_atom, &true_atom, &true_atom}}};
    expr or_t_f_t{expr::functor{"or", {&true_atom, &false_atom, &true_atom}}};
    expr or_f_t_t{expr::functor{"or", {&false_atom, &true_atom, &true_atom}}};
    expr or_f_f_f{expr::functor{"or", {&false_atom, &false_atom, &false_atom}}};
    database.push(rule{&bool_true, {}});
    database.push(rule{&bool_false, {}});
    database.push(rule{&not_true_false, {}});
    database.push(rule{&not_false_true, {}});
    database.push(rule{&or_t_t_t, {}});
    database.push(rule{&or_t_f_t, {}});
    database.push(rule{&or_f_t_t, {}});
    database.push(rule{&or_f_f_f, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_p = manifest.var_sequencer_.next();
    const uint32_t idx_q = manifest.var_sequencer_.next();
    const uint32_t idx_np = manifest.var_sequencer_.next();
    const expr* var_p = manifest.expr_pool_.make(idx_p);
    const expr* var_q = manifest.expr_pool_.make(idx_q);
    const expr* var_np = manifest.expr_pool_.make(idx_np);
    const expr* true_goal = manifest.expr_pool_.make("true", {});
    initial_goals.push(manifest.expr_pool_.make("bool", {var_p}));
    initial_goals.push(manifest.expr_pool_.make("bool", {var_q}));
    initial_goals.push(manifest.expr_pool_.make("or", {var_p, var_q, true_goal}));
    initial_goals.push(manifest.expr_pool_.make("not", {var_p, var_np}));
    initial_goals.push(manifest.expr_pool_.make("or", {var_np, var_q, true_goal}));

    auto sm = manifest.solver_.solve();
    std::set<std::string> p_values;
    size_t solution_count = 0;
    while (solution_count < 2u) {
        sm.resume();
        ASSERT_TRUE(sm.has_yield());
        if (sm.consume_yield() != sim_termination::solved)
            continue;
        ++solution_count;
        const expr* q_val = normalizer.normalize(manifest.expr_pool_.make(idx_q));
        const expr* p_val = normalizer.normalize(manifest.expr_pool_.make(idx_p));
        ASSERT_TRUE(std::holds_alternative<expr::functor>(q_val->content));
        ASSERT_TRUE(std::holds_alternative<expr::functor>(p_val->content));
        EXPECT_EQ(std::get<expr::functor>(q_val->content).name, "true");
        p_values.insert(std::get<expr::functor>(p_val->content).name);
    }
    EXPECT_EQ(p_values.size(), 2u);
    while (true) {
        sm.resume();
        if (!sm.has_yield())
            break;
        if (sm.consume_yield() == sim_termination::solved)
            FAIL() << "unexpected extra solution after exhausting search space";
    }
}

TEST_F(BasicManifestIntegrationTest, RidgeEnumeratesTwoPathTwoColorings) {
    /*
     * Intent: 2-color a 3-node path A-B-C — property checks, not set enumeration.
     * Ported from old-main-test.cpp test_ridge() test 9.
     * initial goals: color(A). color(B). color(C). diff(A,B). diff(B,C).
     * rules: color(red/blue). diff(red,blue). diff(blue,red).
     */
    expr red{expr::functor{"red", {}}};
    expr blue{expr::functor{"blue", {}}};
    expr color_red{expr::functor{"color", {&red}}};
    expr color_blue{expr::functor{"color", {&blue}}};
    expr diff_red_blue{expr::functor{"diff", {&red, &blue}}};
    expr diff_blue_red{expr::functor{"diff", {&blue, &red}}};
    database.push(rule{&color_red, {}});
    database.push(rule{&color_blue, {}});
    database.push(rule{&diff_red_blue, {}});
    database.push(rule{&diff_blue_red, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const uint32_t idx_b = manifest.var_sequencer_.next();
    const uint32_t idx_c = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    const expr* var_b = manifest.expr_pool_.make(idx_b);
    const expr* var_c = manifest.expr_pool_.make(idx_c);
    initial_goals.push(manifest.expr_pool_.make("color", {var_a}));
    initial_goals.push(manifest.expr_pool_.make("color", {var_b}));
    initial_goals.push(manifest.expr_pool_.make("color", {var_c}));
    initial_goals.push(manifest.expr_pool_.make("diff", {var_a, var_b}));
    initial_goals.push(manifest.expr_pool_.make("diff", {var_b, var_c}));

    auto is_valid_color = [](const std::string& s) {
        return s == "red" || s == "blue";
    };

    auto sm = manifest.solver_.solve();
    std::string a1;
    size_t found = 0;
    while (found < 2u) {
        sm.resume();
        ASSERT_TRUE(sm.has_yield());
        if (sm.consume_yield() != sim_termination::solved)
            continue;
        const std::string a_str =
            std::get<expr::functor>(normalizer.normalize(manifest.expr_pool_.make(idx_a))->content).name;
        const std::string b_str =
            std::get<expr::functor>(normalizer.normalize(manifest.expr_pool_.make(idx_b))->content).name;
        const std::string c_str =
            std::get<expr::functor>(normalizer.normalize(manifest.expr_pool_.make(idx_c))->content).name;
        ASSERT_TRUE(is_valid_color(a_str));
        ASSERT_TRUE(is_valid_color(b_str));
        ASSERT_TRUE(is_valid_color(c_str));
        ASSERT_NE(a_str, b_str);
        ASSERT_NE(b_str, c_str);
        ASSERT_EQ(a_str, c_str);
        if (found == 0)
            a1 = a_str;
        else
            ASSERT_NE(a_str, a1);
        ++found;
    }
    while (true) {
        sm.resume();
        if (!sm.has_yield())
            break;
        if (sm.consume_yield() == sim_termination::solved)
            FAIL() << "unexpected extra solution";
    }
}

TEST_F(BasicManifestIntegrationTest, RidgeEnumeratesK3ThreeColorings) {
    /*
     * Intent: enumerate all 3! proper colorings of K3 over colors {red, green, blue}.
     * Ported from old-main-test.cpp test_ridge() test 10.
     * initial goals: color(A). color(B). color(C). diff(A,B). diff(A,C). diff(B,C).
     * rules: color(red/green/blue). diff(x,y) for all ordered x != y pairs.
     */
    static constexpr std::array<uint32_t, 10> kSeeds = {
        0, 1, 7, 13, 42, 99, 123, 256, 1000, 9999};

    for (uint32_t seed : kSeeds) {
        SCOPED_TRACE(testing::Message() << "seed=" << seed);

        db seed_db;
        initial_goal_exprs seed_goals;
        trail seed_trail;
        locator seed_loc;
        seed_loc.bind_as<i_log_to_current_trail_frame>(seed_trail);
        expr_pool seed_pool{seed_loc};

        expr red{expr::functor{"red", {}}};
        expr green{expr::functor{"green", {}}};
        expr blue{expr::functor{"blue", {}}};
        expr color_red{expr::functor{"color", {&red}}};
        expr color_green{expr::functor{"color", {&green}}};
        expr color_blue{expr::functor{"color", {&blue}}};
        seed_db.push(rule{&color_red, {}});
        seed_db.push(rule{&color_green, {}});
        seed_db.push(rule{&color_blue, {}});

        expr diff_red_green{expr::functor{"diff", {&red, &green}}};
        expr diff_red_blue{expr::functor{"diff", {&red, &blue}}};
        expr diff_green_red{expr::functor{"diff", {&green, &red}}};
        expr diff_green_blue{expr::functor{"diff", {&green, &blue}}};
        expr diff_blue_red{expr::functor{"diff", {&blue, &red}}};
        expr diff_blue_green{expr::functor{"diff", {&blue, &green}}};
        seed_db.push(rule{&diff_red_green, {}});
        seed_db.push(rule{&diff_red_blue, {}});
        seed_db.push(rule{&diff_green_red, {}});
        seed_db.push(rule{&diff_green_blue, {}});
        seed_db.push(rule{&diff_blue_red, {}});
        seed_db.push(rule{&diff_blue_green, {}});

        basic_manifest manifest{seed_db, seed_goals, 128, seed};
        normalizer seed_normalizer{manifest.loc_};
        const uint32_t idx_a = manifest.var_sequencer_.next();
        const uint32_t idx_b = manifest.var_sequencer_.next();
        const uint32_t idx_c = manifest.var_sequencer_.next();
        const expr* var_a = manifest.expr_pool_.make(idx_a);
        const expr* var_b = manifest.expr_pool_.make(idx_b);
        const expr* var_c = manifest.expr_pool_.make(idx_c);
        seed_goals.push(manifest.expr_pool_.make("color", {var_a}));
        seed_goals.push(manifest.expr_pool_.make("color", {var_b}));
        seed_goals.push(manifest.expr_pool_.make("color", {var_c}));
        seed_goals.push(manifest.expr_pool_.make("diff", {var_a, var_b}));
        seed_goals.push(manifest.expr_pool_.make("diff", {var_a, var_c}));
        seed_goals.push(manifest.expr_pool_.make("diff", {var_b, var_c}));

        const expr* r = seed_pool.import(&red);
        const expr* g = seed_pool.import(&green);
        const expr* b = seed_pool.import(&blue);
        std::set<solution> expected = {
            {r, g, b}, {r, b, g}, {g, r, b}, {g, b, r}, {b, r, g}, {b, g, r},
        };

        next_until_refuted(
            manifest.solver_,
            expected,
            [&]() -> solution {
                return {
                    seed_pool.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_a))),
                    seed_pool.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_b))),
                    seed_pool.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_c))),
                };
            });
    }
}

TEST_F(BasicManifestIntegrationTest, RidgeEnumeratesK3TailFourNodeColorings) {
    /*
     * Intent: 12 colorings — K3 on (A,B,C) plus tail D with diff(A,D).
     * Ported from old-main-test.cpp test_ridge() test 12.
     */
    static constexpr std::array<uint32_t, 10> kSeeds = {
        0, 1, 7, 13, 42, 99, 123, 256, 1000, 9999};
    for (uint32_t seed : kSeeds) {
        SCOPED_TRACE(testing::Message() << "seed=" << seed);
        db seed_db;
        initial_goal_exprs seed_goals;
        trail seed_trail;
        locator seed_loc;
        seed_loc.bind_as<i_log_to_current_trail_frame>(seed_trail);
        expr_pool seed_pool{seed_loc};
        expr red{expr::functor{"red", {}}};
        expr green{expr::functor{"green", {}}};
        expr blue{expr::functor{"blue", {}}};
        expr color_red{expr::functor{"color", {&red}}};
        expr color_green{expr::functor{"color", {&green}}};
        expr color_blue{expr::functor{"color", {&blue}}};
        seed_db.push(rule{&color_red, {}});
        seed_db.push(rule{&color_green, {}});
        seed_db.push(rule{&color_blue, {}});
        expr diff_rg{expr::functor{"diff", {&red, &green}}};
        expr diff_rb{expr::functor{"diff", {&red, &blue}}};
        expr diff_gr{expr::functor{"diff", {&green, &red}}};
        expr diff_gb{expr::functor{"diff", {&green, &blue}}};
        expr diff_br{expr::functor{"diff", {&blue, &red}}};
        expr diff_bg{expr::functor{"diff", {&blue, &green}}};
        seed_db.push(rule{&diff_rg, {}});
        seed_db.push(rule{&diff_rb, {}});
        seed_db.push(rule{&diff_gr, {}});
        seed_db.push(rule{&diff_gb, {}});
        seed_db.push(rule{&diff_br, {}});
        seed_db.push(rule{&diff_bg, {}});
        basic_manifest manifest{seed_db, seed_goals, 128, seed};
        normalizer seed_normalizer{manifest.loc_};
        const uint32_t idx_a = manifest.var_sequencer_.next();
        const uint32_t idx_b = manifest.var_sequencer_.next();
        const uint32_t idx_c = manifest.var_sequencer_.next();
        const uint32_t idx_d = manifest.var_sequencer_.next();
        const expr* var_a = manifest.expr_pool_.make(idx_a);
        const expr* var_b = manifest.expr_pool_.make(idx_b);
        const expr* var_c = manifest.expr_pool_.make(idx_c);
        const expr* var_d = manifest.expr_pool_.make(idx_d);
        seed_goals.push(manifest.expr_pool_.make("color", {var_a}));
        seed_goals.push(manifest.expr_pool_.make("color", {var_b}));
        seed_goals.push(manifest.expr_pool_.make("color", {var_c}));
        seed_goals.push(manifest.expr_pool_.make("color", {var_d}));
        seed_goals.push(manifest.expr_pool_.make("diff", {var_a, var_b}));
        seed_goals.push(manifest.expr_pool_.make("diff", {var_a, var_c}));
        seed_goals.push(manifest.expr_pool_.make("diff", {var_b, var_c}));
        seed_goals.push(manifest.expr_pool_.make("diff", {var_a, var_d}));
        const expr* r = seed_pool.import(&red);
        const expr* g = seed_pool.import(&green);
        const expr* b = seed_pool.import(&blue);
        std::set<solution> expected = {
            {r, g, b, g}, {r, g, b, b}, {r, b, g, g}, {r, b, g, b},
            {g, r, b, r}, {g, r, b, b}, {g, b, r, r}, {g, b, r, b},
            {b, r, g, r}, {b, r, g, g}, {b, g, r, r}, {b, g, r, g},
        };
        next_until_refuted(manifest.solver_, expected, [&]() -> solution {
            return {
                seed_pool.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_a))),
                seed_pool.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_b))),
                seed_pool.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_c))),
                seed_pool.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_d))),
            };
        });
    }
}

TEST_F(BasicManifestIntegrationTest, RidgeEnumeratesFourVarSatThreeClauses) {
    /*
     * Intent: satisfy (P ∨ Q) ∧ (R ∨ S) ∧ (¬P ∨ ¬R).
     * Ported from old-main-test.cpp test_ridge() test 13.
     */
    static constexpr std::array<uint32_t, 10> kSeeds = {
        0, 1, 7, 13, 42, 99, 123, 256, 1000, 9999};
    static constexpr size_t kSatBudget = 256;

    for (uint32_t seed : kSeeds) {
        SCOPED_TRACE(testing::Message() << "seed=" << seed);

        db seed_db;
        initial_goal_exprs seed_goals;
        trail seed_trail;
        locator seed_loc;
        seed_loc.bind_as<i_log_to_current_trail_frame>(seed_trail);
        expr_pool seed_pool{seed_loc};

        expr true_atom{expr::functor{"true", {}}};
        expr false_atom{expr::functor{"false", {}}};
        expr bool_true{expr::functor{"bool", {&true_atom}}};
        expr bool_false{expr::functor{"bool", {&false_atom}}};
        expr not_true_false{expr::functor{"not", {&true_atom, &false_atom}}};
        expr not_false_true{expr::functor{"not", {&false_atom, &true_atom}}};
        seed_db.push(rule{&bool_true, {}});
        seed_db.push(rule{&bool_false, {}});
        seed_db.push(rule{&not_true_false, {}});
        seed_db.push(rule{&not_false_true, {}});

        expr or_t_t_t{expr::functor{"or", {&true_atom, &true_atom, &true_atom}}};
        expr or_t_f_t{expr::functor{"or", {&true_atom, &false_atom, &true_atom}}};
        expr or_f_t_t{expr::functor{"or", {&false_atom, &true_atom, &true_atom}}};
        expr or_f_f_f{expr::functor{"or", {&false_atom, &false_atom, &false_atom}}};
        seed_db.push(rule{&or_t_t_t, {}});
        seed_db.push(rule{&or_t_f_t, {}});
        seed_db.push(rule{&or_f_t_t, {}});
        seed_db.push(rule{&or_f_f_f, {}});

        expr and_t_t_t{expr::functor{"and", {&true_atom, &true_atom, &true_atom}}};
        expr and_t_f_f{expr::functor{"and", {&true_atom, &false_atom, &false_atom}}};
        expr and_f_t_f{expr::functor{"and", {&false_atom, &true_atom, &false_atom}}};
        expr and_f_f_f{expr::functor{"and", {&false_atom, &false_atom, &false_atom}}};
        seed_db.push(rule{&and_t_t_t, {}});
        seed_db.push(rule{&and_t_f_f, {}});
        seed_db.push(rule{&and_f_t_f, {}});
        seed_db.push(rule{&and_f_f_f, {}});

        basic_manifest manifest{seed_db, seed_goals, kSatBudget, seed};
        normalizer seed_normalizer{manifest.loc_};
        const uint32_t idx_p = manifest.var_sequencer_.next();
        const uint32_t idx_q = manifest.var_sequencer_.next();
        const uint32_t idx_r = manifest.var_sequencer_.next();
        const uint32_t idx_s = manifest.var_sequencer_.next();
        const uint32_t idx_pq = manifest.var_sequencer_.next();
        const uint32_t idx_rs = manifest.var_sequencer_.next();
        const uint32_t idx_np = manifest.var_sequencer_.next();
        const uint32_t idx_nr = manifest.var_sequencer_.next();
        const uint32_t idx_npr = manifest.var_sequencer_.next();
        const uint32_t idx_pq_rs = manifest.var_sequencer_.next();
        const expr* var_p = manifest.expr_pool_.make(idx_p);
        const expr* var_q = manifest.expr_pool_.make(idx_q);
        const expr* var_r = manifest.expr_pool_.make(idx_r);
        const expr* var_s = manifest.expr_pool_.make(idx_s);
        const expr* var_pq = manifest.expr_pool_.make(idx_pq);
        const expr* var_rs = manifest.expr_pool_.make(idx_rs);
        const expr* var_np = manifest.expr_pool_.make(idx_np);
        const expr* var_nr = manifest.expr_pool_.make(idx_nr);
        const expr* var_npr = manifest.expr_pool_.make(idx_npr);
        const expr* var_pq_rs = manifest.expr_pool_.make(idx_pq_rs);
        const expr* true_goal = manifest.expr_pool_.make("true", {});
        seed_goals.push(manifest.expr_pool_.make("bool", {var_p}));
        seed_goals.push(manifest.expr_pool_.make("bool", {var_q}));
        seed_goals.push(manifest.expr_pool_.make("bool", {var_r}));
        seed_goals.push(manifest.expr_pool_.make("bool", {var_s}));
        seed_goals.push(manifest.expr_pool_.make("or", {var_p, var_q, var_pq}));
        seed_goals.push(manifest.expr_pool_.make("or", {var_r, var_s, var_rs}));
        seed_goals.push(manifest.expr_pool_.make("not", {var_p, var_np}));
        seed_goals.push(manifest.expr_pool_.make("not", {var_r, var_nr}));
        seed_goals.push(manifest.expr_pool_.make("or", {var_np, var_nr, var_npr}));
        seed_goals.push(manifest.expr_pool_.make("and", {var_pq, var_rs, var_pq_rs}));
        seed_goals.push(manifest.expr_pool_.make("and", {var_pq_rs, var_npr, true_goal}));

        const expr* t_saved = seed_pool.import(&true_atom);
        const expr* f_saved = seed_pool.import(&false_atom);
        std::set<solution> expected = {
            {t_saved, t_saved, f_saved, t_saved},
            {t_saved, f_saved, f_saved, t_saved},
            {f_saved, t_saved, t_saved, t_saved},
            {f_saved, t_saved, t_saved, f_saved},
            {f_saved, t_saved, f_saved, t_saved},
        };

        next_until_refuted(
            manifest.solver_,
            expected,
            [&]() -> solution {
                return {
                    seed_pool.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_p))),
                    seed_pool.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_q))),
                    seed_pool.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_r))),
                    seed_pool.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_s))),
                };
            });
    }
}

TEST_F(BasicManifestIntegrationTest, RidgeEnumeratesAddPairsSummingLessThanTen) {
    /*
     * Intent: enumerate all (X,Y) where add(X,Y,S) and lt(S,10) (55 pairs).
     * Ported from old-main-test.cpp test_ridge() test 15.
     */
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    expr zero{expr::functor{"zero", {}}};
    expr nat_zero{expr::functor{"nat", {&zero}}};
    database.push(rule{&nat_zero, {}});

    expr rv1{expr::var{0}};
    expr suc_rv1{expr::functor{"suc", {&rv1}}};
    expr nat_suc{expr::functor{"nat", {&suc_rv1}}};
    expr nat_rv1{expr::functor{"nat", {&rv1}}};
    database.push(rule{&nat_suc, {&nat_rv1}});

    expr rv2{expr::var{0}};
    expr add_zero_y_y{expr::functor{"add", {&zero, &rv2, &rv2}}};
    expr nat_rv2{expr::functor{"nat", {&rv2}}};
    database.push(rule{&add_zero_y_y, {&nat_rv2}});

    expr rv3{expr::var{0}};
    expr rv4{expr::var{1}};
    expr rv5{expr::var{2}};
    expr suc_rv3{expr::functor{"suc", {&rv3}}};
    expr suc_rv5{expr::functor{"suc", {&rv5}}};
    expr add_suc{expr::functor{"add", {&suc_rv3, &rv4, &suc_rv5}}};
    expr add_body{expr::functor{"add", {&rv3, &rv4, &rv5}}};
    database.push(rule{&add_suc, {&add_body}});

    expr rv6{expr::var{0}};
    expr suc_rv6{expr::functor{"suc", {&rv6}}};
    expr lt_zero_suc{expr::functor{"lt", {&zero, &suc_rv6}}};
    expr nat_rv6{expr::functor{"nat", {&rv6}}};
    database.push(rule{&lt_zero_suc, {&nat_rv6}});

    expr rv7{expr::var{0}};
    expr rv8{expr::var{1}};
    expr suc_rv7{expr::functor{"suc", {&rv7}}};
    expr suc_rv8{expr::functor{"suc", {&rv8}}};
    expr lt_suc_suc{expr::functor{"lt", {&suc_rv7, &suc_rv8}}};
    expr lt_body{expr::functor{"lt", {&rv7, &rv8}}};
    database.push(rule{&lt_suc_suc, {&lt_body}});

    std::set<solution> expected;
    for (int x = 0; x < 10; ++x) {
        for (int y = 0; y < 10 - x; ++y)
            expected.insert({peano_saved(x), peano_saved(y)});
    }
    ASSERT_EQ(expected.size(), 55u);

    db probe_db;
    initial_goal_exprs probe_goals;
    probe_db.push(rule{&nat_zero, {}});
    probe_db.push(rule{&nat_suc, {&nat_rv1}});
    probe_db.push(rule{&add_zero_y_y, {&nat_rv2}});
    probe_db.push(rule{&add_suc, {&add_body}});
    probe_db.push(rule{&lt_zero_suc, {&nat_rv6}});
    probe_db.push(rule{&lt_suc_suc, {&lt_body}});

    basic_manifest probe_manifest{probe_db, probe_goals, kPeanoBudget, kSeed};
    normalizer probe_normalizer{probe_manifest.loc_};
    const uint32_t idx_x_probe = probe_manifest.var_sequencer_.next();
    const uint32_t idx_y_probe = probe_manifest.var_sequencer_.next();
    const uint32_t idx_s_probe = probe_manifest.var_sequencer_.next();
    const expr* var_x_probe = probe_manifest.expr_pool_.make(idx_x_probe);
    const expr* var_y_probe = probe_manifest.expr_pool_.make(idx_y_probe);
    const expr* var_s_probe = probe_manifest.expr_pool_.make(idx_s_probe);
    const expr* ten_probe = probe_manifest.expr_pool_.make("zero", {});
    for (int i = 0; i < 10; ++i)
        ten_probe = probe_manifest.expr_pool_.make("suc", {ten_probe});
    probe_goals.push(probe_manifest.expr_pool_.make("add", {var_x_probe, var_y_probe, var_s_probe}));
    probe_goals.push(probe_manifest.expr_pool_.make("lt", {var_s_probe, ten_probe}));

    auto probe_sm = probe_manifest.solver_.solve();
    std::set<solution> probe_remaining = expected;
    std::set<solution> probe_visited;
    while (!probe_remaining.empty()) {
        probe_sm.resume();
        ASSERT_TRUE(probe_sm.has_yield()) << "solver stopped before all expected solutions found";
        const auto term = probe_sm.consume_yield();
        ASSERT_NE(term, sim_termination::depth_exceeded) << "raise kPeanoBudget";
        if (term != sim_termination::solved)
            continue;
        const solution s = {
            saved_expr_pool_.import(probe_normalizer.normalize(probe_manifest.expr_pool_.make(idx_x_probe))),
            saved_expr_pool_.import(probe_normalizer.normalize(probe_manifest.expr_pool_.make(idx_y_probe))),
        };
        if (probe_visited.count(s))
            continue;
        probe_visited.insert(s);
        auto it = probe_remaining.find(s);
        ASSERT_NE(it, probe_remaining.end()) << "unexpected solution";
        probe_remaining.erase(it);
    }

    db solve_db;
    initial_goal_exprs solve_goals;
    solve_db.push(rule{&nat_zero, {}});
    solve_db.push(rule{&nat_suc, {&nat_rv1}});
    solve_db.push(rule{&add_zero_y_y, {&nat_rv2}});
    solve_db.push(rule{&add_suc, {&add_body}});
    solve_db.push(rule{&lt_zero_suc, {&nat_rv6}});
    solve_db.push(rule{&lt_suc_suc, {&lt_body}});

    basic_manifest manifest{solve_db, solve_goals, kPeanoBudget, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_x = manifest.var_sequencer_.next();
    const uint32_t idx_y = manifest.var_sequencer_.next();
    const uint32_t idx_s = manifest.var_sequencer_.next();
    const expr* var_x = manifest.expr_pool_.make(idx_x);
    const expr* var_y = manifest.expr_pool_.make(idx_y);
    const expr* var_s = manifest.expr_pool_.make(idx_s);
    const expr* ten = manifest.expr_pool_.make("zero", {});
    for (int i = 0; i < 10; ++i)
        ten = manifest.expr_pool_.make("suc", {ten});
    solve_goals.push(manifest.expr_pool_.make("add", {var_x, var_y, var_s}));
    solve_goals.push(manifest.expr_pool_.make("lt", {var_s, ten}));

    next_until_refuted(
        manifest.solver_,
        expected,
        [&]() -> solution {
            return {
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_x))),
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_y))),
            };
        });
}

TEST_F(BasicManifestIntegrationTest, RidgeEnumeratesAddPairsSummingExactlyTen) {
    /*
     * Intent: enumerate all (X,Y) where add(X,Y,10) (11 pairs).
     * Ported from old-main-test.cpp test_ridge() test 16.
     */
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    expr zero{expr::functor{"zero", {}}};
    expr nat_zero{expr::functor{"nat", {&zero}}};
    database.push(rule{&nat_zero, {}});

    expr rv1{expr::var{0}};
    expr suc_rv1{expr::functor{"suc", {&rv1}}};
    expr nat_suc{expr::functor{"nat", {&suc_rv1}}};
    expr nat_rv1{expr::functor{"nat", {&rv1}}};
    database.push(rule{&nat_suc, {&nat_rv1}});

    expr rv2{expr::var{0}};
    expr add_zero_y_y{expr::functor{"add", {&zero, &rv2, &rv2}}};
    expr nat_rv2{expr::functor{"nat", {&rv2}}};
    database.push(rule{&add_zero_y_y, {&nat_rv2}});

    expr rv3{expr::var{0}};
    expr rv4{expr::var{1}};
    expr rv5{expr::var{2}};
    expr suc_rv3{expr::functor{"suc", {&rv3}}};
    expr suc_rv5{expr::functor{"suc", {&rv5}}};
    expr add_suc{expr::functor{"add", {&suc_rv3, &rv4, &suc_rv5}}};
    expr add_body{expr::functor{"add", {&rv3, &rv4, &rv5}}};
    database.push(rule{&add_suc, {&add_body}});

    std::set<solution> expected;
    for (int x = 0; x <= 10; ++x)
        expected.insert({peano_saved(x), peano_saved(10 - x)});
    ASSERT_EQ(expected.size(), 11u);

    basic_manifest manifest{database, initial_goals, kPeanoBudget, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_x = manifest.var_sequencer_.next();
    const uint32_t idx_y = manifest.var_sequencer_.next();
    const expr* var_x = manifest.expr_pool_.make(idx_x);
    const expr* var_y = manifest.expr_pool_.make(idx_y);
    const expr* ten = manifest.expr_pool_.make("zero", {});
    for (int i = 0; i < 10; ++i)
        ten = manifest.expr_pool_.make("suc", {ten});
    initial_goals.push(manifest.expr_pool_.make("add", {var_x, var_y, ten}));

    next_until_refuted(
        manifest.solver_,
        expected,
        [&]() -> solution {
            return {
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_x))),
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_y))),
            };
        });
}

TEST_F(BasicManifestIntegrationTest, RidgeEnumeratesMulPairsProductEight) {
    /*
     * Intent: enumerate all factor pairs (X,Y) where mul(X,Y,8) (4 pairs).
     * Ported from old-main-test.cpp test_ridge() test 17.
     */
    static constexpr size_t kPeanoBudget = 256;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    expr zero{expr::functor{"zero", {}}};
    expr nat_zero{expr::functor{"nat", {&zero}}};
    database.push(rule{&nat_zero, {}});

    expr rv1{expr::var{0}};
    expr suc_rv1{expr::functor{"suc", {&rv1}}};
    expr nat_suc{expr::functor{"nat", {&suc_rv1}}};
    expr nat_rv1{expr::functor{"nat", {&rv1}}};
    database.push(rule{&nat_suc, {&nat_rv1}});

    expr rv2{expr::var{0}};
    expr add_zero_y_y{expr::functor{"add", {&zero, &rv2, &rv2}}};
    expr nat_rv2{expr::functor{"nat", {&rv2}}};
    database.push(rule{&add_zero_y_y, {&nat_rv2}});

    expr rv3{expr::var{0}};
    expr rv4{expr::var{1}};
    expr rv5{expr::var{2}};
    expr suc_rv3{expr::functor{"suc", {&rv3}}};
    expr suc_rv5{expr::functor{"suc", {&rv5}}};
    expr add_suc{expr::functor{"add", {&suc_rv3, &rv4, &suc_rv5}}};
    expr add_body{expr::functor{"add", {&rv3, &rv4, &rv5}}};
    database.push(rule{&add_suc, {&add_body}});

    expr rv6{expr::var{0}};
    expr mul_zero_y_zero{expr::functor{"mul", {&zero, &rv6, &zero}}};
    expr nat_rv6{expr::functor{"nat", {&rv6}}};
    database.push(rule{&mul_zero_y_zero, {&nat_rv6}});

    expr rv7{expr::var{0}};
    expr rv8{expr::var{1}};
    expr rv9{expr::var{2}};
    expr rv10{expr::var{3}};
    expr suc_rv7{expr::functor{"suc", {&rv7}}};
    expr mul_suc{expr::functor{"mul", {&suc_rv7, &rv8, &rv9}}};
    expr mul_body{expr::functor{"mul", {&rv7, &rv8, &rv10}}};
    expr add_body2{expr::functor{"add", {&rv10, &rv8, &rv9}}};
    database.push(rule{&mul_suc, {&mul_body, &add_body2}});

    std::set<solution> expected = {
        {peano_saved(1), peano_saved(8)},
        {peano_saved(2), peano_saved(4)},
        {peano_saved(4), peano_saved(2)},
        {peano_saved(8), peano_saved(1)},
    };

    basic_manifest manifest{database, initial_goals, kPeanoBudget, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_x = manifest.var_sequencer_.next();
    const uint32_t idx_y = manifest.var_sequencer_.next();
    const expr* var_x = manifest.expr_pool_.make(idx_x);
    const expr* var_y = manifest.expr_pool_.make(idx_y);
    const expr* eight = manifest.expr_pool_.make("zero", {});
    for (int i = 0; i < 8; ++i)
        eight = manifest.expr_pool_.make("suc", {eight});
    initial_goals.push(manifest.expr_pool_.make("mul", {var_x, var_y, eight}));

    next_until_refuted(
        manifest.solver_,
        expected,
        [&]() -> solution {
            return {
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_x))),
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_y))),
            };
        });
}

TEST_F(BasicManifestIntegrationTest, RidgeEnumeratesDualBoundedSharedXSums) {
    /*
     * Intent: enumerate all triples (X,Y,Z) with add(X,Y,S), add(X,Z,T), lt(S,4), lt(T,4).
     * Ported from old-main-test.cpp test_ridge() test 18.
     */
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    expr zero{expr::functor{"zero", {}}};
    expr nat_zero{expr::functor{"nat", {&zero}}};
    database.push(rule{&nat_zero, {}});

    expr rv1{expr::var{0}};
    expr suc_rv1{expr::functor{"suc", {&rv1}}};
    expr nat_suc{expr::functor{"nat", {&suc_rv1}}};
    expr nat_rv1{expr::functor{"nat", {&rv1}}};
    database.push(rule{&nat_suc, {&nat_rv1}});

    expr rv2{expr::var{0}};
    expr add_zero_y_y{expr::functor{"add", {&zero, &rv2, &rv2}}};
    expr nat_rv2{expr::functor{"nat", {&rv2}}};
    database.push(rule{&add_zero_y_y, {&nat_rv2}});

    expr rv3{expr::var{0}};
    expr rv4{expr::var{1}};
    expr rv5{expr::var{2}};
    expr suc_rv3{expr::functor{"suc", {&rv3}}};
    expr suc_rv5{expr::functor{"suc", {&rv5}}};
    expr add_suc{expr::functor{"add", {&suc_rv3, &rv4, &suc_rv5}}};
    expr add_body{expr::functor{"add", {&rv3, &rv4, &rv5}}};
    database.push(rule{&add_suc, {&add_body}});

    expr rv6{expr::var{0}};
    expr suc_rv6{expr::functor{"suc", {&rv6}}};
    expr lt_zero_suc{expr::functor{"lt", {&zero, &suc_rv6}}};
    expr nat_rv6{expr::functor{"nat", {&rv6}}};
    database.push(rule{&lt_zero_suc, {&nat_rv6}});

    expr rv7{expr::var{0}};
    expr rv8{expr::var{1}};
    expr suc_rv7{expr::functor{"suc", {&rv7}}};
    expr suc_rv8{expr::functor{"suc", {&rv8}}};
    expr lt_suc_suc{expr::functor{"lt", {&suc_rv7, &suc_rv8}}};
    expr lt_body{expr::functor{"lt", {&rv7, &rv8}}};
    database.push(rule{&lt_suc_suc, {&lt_body}});

    std::set<solution> expected;
    for (int x = 0; x < 4; ++x) {
        for (int y = 0; y < 4 - x; ++y) {
            for (int z = 0; z < 4 - x; ++z)
                expected.insert({peano_saved(x), peano_saved(y), peano_saved(z)});
        }
    }
    ASSERT_EQ(expected.size(), 30u);

    basic_manifest manifest{database, initial_goals, kPeanoBudget, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_x = manifest.var_sequencer_.next();
    const uint32_t idx_y = manifest.var_sequencer_.next();
    const uint32_t idx_z = manifest.var_sequencer_.next();
    const uint32_t idx_s = manifest.var_sequencer_.next();
    const uint32_t idx_t = manifest.var_sequencer_.next();
    const expr* var_x = manifest.expr_pool_.make(idx_x);
    const expr* var_y = manifest.expr_pool_.make(idx_y);
    const expr* var_z = manifest.expr_pool_.make(idx_z);
    const expr* var_s = manifest.expr_pool_.make(idx_s);
    const expr* var_t = manifest.expr_pool_.make(idx_t);
    const expr* bound = manifest.expr_pool_.make("zero", {});
    for (int i = 0; i < 4; ++i)
        bound = manifest.expr_pool_.make("suc", {bound});
    initial_goals.push(manifest.expr_pool_.make("add", {var_x, var_y, var_s}));
    initial_goals.push(manifest.expr_pool_.make("add", {var_x, var_z, var_t}));
    initial_goals.push(manifest.expr_pool_.make("lt", {var_s, bound}));
    initial_goals.push(manifest.expr_pool_.make("lt", {var_t, bound}));

    next_until_refuted(
        manifest.solver_,
        expected,
        [&]() -> solution {
            return {
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_x))),
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_y))),
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_z))),
            };
        });
}

TEST_F(BasicManifestIntegrationTest, RidgeEnumeratesCatalanTreesWithFiveNodes) {
    /*
     * Intent: enumerate all wf(T) trees with exactly five nodes (Catalan C5 = 42).
     * Ported from old-main-test.cpp test_ridge() test 19.
     */
    static constexpr size_t kCatalanBudget = 70;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };
    auto bin_saved = [&](const expr* lhs, const expr* rhs) -> const expr* {
        return saved_expr_pool_.make("bin", {lhs, rhs});
    };

    const expr* nil_saved = saved_expr_pool_.make("nil", {});
    const expr* s1 = bin_saved(nil_saved, nil_saved);
    const expr* s2_left_chain = bin_saved(nil_saved, s1);
    const expr* s2_right_chain = bin_saved(s1, nil_saved);

    const expr* s3_0 = bin_saved(nil_saved, s2_left_chain);
    const expr* s3_1 = bin_saved(nil_saved, s2_right_chain);
    const expr* s3_2 = bin_saved(s1, s1);
    const expr* s3_3 = bin_saved(s2_left_chain, nil_saved);
    const expr* s3_4 = bin_saved(s2_right_chain, nil_saved);

    const expr* s4_0 = bin_saved(nil_saved, s3_0);
    const expr* s4_1 = bin_saved(nil_saved, s3_1);
    const expr* s4_2 = bin_saved(nil_saved, s3_2);
    const expr* s4_3 = bin_saved(nil_saved, s3_3);
    const expr* s4_4 = bin_saved(nil_saved, s3_4);
    const expr* s4_5 = bin_saved(s1, s2_left_chain);
    const expr* s4_6 = bin_saved(s1, s2_right_chain);
    const expr* s4_7 = bin_saved(s2_left_chain, s1);
    const expr* s4_8 = bin_saved(s2_right_chain, s1);
    const expr* s4_9 = bin_saved(s3_0, nil_saved);
    const expr* s4_10 = bin_saved(s3_1, nil_saved);
    const expr* s4_11 = bin_saved(s3_2, nil_saved);
    const expr* s4_12 = bin_saved(s3_3, nil_saved);
    const expr* s4_13 = bin_saved(s3_4, nil_saved);

    std::set<solution> expected;
    expected.insert({bin_saved(nil_saved, s4_0)});
    expected.insert({bin_saved(nil_saved, s4_1)});
    expected.insert({bin_saved(nil_saved, s4_2)});
    expected.insert({bin_saved(nil_saved, s4_3)});
    expected.insert({bin_saved(nil_saved, s4_4)});
    expected.insert({bin_saved(nil_saved, s4_5)});
    expected.insert({bin_saved(nil_saved, s4_6)});
    expected.insert({bin_saved(nil_saved, s4_7)});
    expected.insert({bin_saved(nil_saved, s4_8)});
    expected.insert({bin_saved(nil_saved, s4_9)});
    expected.insert({bin_saved(nil_saved, s4_10)});
    expected.insert({bin_saved(nil_saved, s4_11)});
    expected.insert({bin_saved(nil_saved, s4_12)});
    expected.insert({bin_saved(nil_saved, s4_13)});

    expected.insert({bin_saved(s1, s3_0)});
    expected.insert({bin_saved(s1, s3_1)});
    expected.insert({bin_saved(s1, s3_2)});
    expected.insert({bin_saved(s1, s3_3)});
    expected.insert({bin_saved(s1, s3_4)});

    expected.insert({bin_saved(s2_left_chain, s2_left_chain)});
    expected.insert({bin_saved(s2_left_chain, s2_right_chain)});
    expected.insert({bin_saved(s2_right_chain, s2_left_chain)});
    expected.insert({bin_saved(s2_right_chain, s2_right_chain)});

    expected.insert({bin_saved(s3_0, s1)});
    expected.insert({bin_saved(s3_1, s1)});
    expected.insert({bin_saved(s3_2, s1)});
    expected.insert({bin_saved(s3_3, s1)});
    expected.insert({bin_saved(s3_4, s1)});

    expected.insert({bin_saved(s4_0, nil_saved)});
    expected.insert({bin_saved(s4_1, nil_saved)});
    expected.insert({bin_saved(s4_2, nil_saved)});
    expected.insert({bin_saved(s4_3, nil_saved)});
    expected.insert({bin_saved(s4_4, nil_saved)});
    expected.insert({bin_saved(s4_5, nil_saved)});
    expected.insert({bin_saved(s4_6, nil_saved)});
    expected.insert({bin_saved(s4_7, nil_saved)});
    expected.insert({bin_saved(s4_8, nil_saved)});
    expected.insert({bin_saved(s4_9, nil_saved)});
    expected.insert({bin_saved(s4_10, nil_saved)});
    expected.insert({bin_saved(s4_11, nil_saved)});
    expected.insert({bin_saved(s4_12, nil_saved)});
    expected.insert({bin_saved(s4_13, nil_saved)});
    ASSERT_EQ(expected.size(), 42u);

    expr zero{expr::functor{"zero", {}}};
    expr nat_zero{expr::functor{"nat", {&zero}}};
    database.push(rule{&nat_zero, {}});

    expr rv1{expr::var{0}};
    expr suc_rv1{expr::functor{"suc", {&rv1}}};
    expr nat_suc{expr::functor{"nat", {&suc_rv1}}};
    expr nat_rv1{expr::functor{"nat", {&rv1}}};
    database.push(rule{&nat_suc, {&nat_rv1}});

    expr rv2{expr::var{0}};
    expr add_zero_y_y{expr::functor{"add", {&zero, &rv2, &rv2}}};
    expr nat_rv2{expr::functor{"nat", {&rv2}}};
    database.push(rule{&add_zero_y_y, {&nat_rv2}});

    expr rv3{expr::var{0}};
    expr rv4{expr::var{1}};
    expr rv5{expr::var{2}};
    expr suc_rv3{expr::functor{"suc", {&rv3}}};
    expr suc_rv5{expr::functor{"suc", {&rv5}}};
    expr add_suc{expr::functor{"add", {&suc_rv3, &rv4, &suc_rv5}}};
    expr add_body{expr::functor{"add", {&rv3, &rv4, &rv5}}};
    database.push(rule{&add_suc, {&add_body}});

    expr nil{expr::functor{"nil", {}}};
    expr wf_nil{expr::functor{"wf", {&nil}}};
    database.push(rule{&wf_nil, {}});

    expr rv6{expr::var{0}};
    expr rv7{expr::var{1}};
    expr bin_rv6_rv7{expr::functor{"bin", {&rv6, &rv7}}};
    expr wf_bin{expr::functor{"wf", {&bin_rv6_rv7}}};
    expr wf_left{expr::functor{"wf", {&rv6}}};
    expr wf_right{expr::functor{"wf", {&rv7}}};
    database.push(rule{&wf_bin, {&wf_left, &wf_right}});

    expr nodes_nil_zero{expr::functor{"nodes", {&nil, &zero}}};
    database.push(rule{&nodes_nil_zero, {}});

    expr one{expr::functor{"suc", {&zero}}};
    expr rv8{expr::var{0}};
    expr rv9{expr::var{1}};
    expr rv10{expr::var{2}};
    expr rv11{expr::var{3}};
    expr rv12{expr::var{4}};
    expr rv13{expr::var{5}};
    expr bin_rv8_rv9{expr::functor{"bin", {&rv8, &rv9}}};
    expr nodes_head{expr::functor{"nodes", {&bin_rv8_rv9, &rv10}}};
    expr nodes_left{expr::functor{"nodes", {&rv8, &rv11}}};
    expr nodes_right{expr::functor{"nodes", {&rv9, &rv12}}};
    expr add_sizes{expr::functor{"add", {&rv11, &rv12, &rv13}}};
    expr add_one{expr::functor{"add", {&one, &rv13, &rv10}}};
    database.push(rule{&nodes_head, {&nodes_left, &nodes_right, &add_sizes, &add_one}});

    basic_manifest manifest{database, initial_goals, kCatalanBudget, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_t = manifest.var_sequencer_.next();
    const expr* var_t = manifest.expr_pool_.make(idx_t);
    const expr* five = manifest.expr_pool_.make("zero", {});
    for (int i = 0; i < 5; ++i)
        five = manifest.expr_pool_.make("suc", {five});
    initial_goals.push(manifest.expr_pool_.make("wf", {var_t}));
    initial_goals.push(manifest.expr_pool_.make("nodes", {var_t, five}));

    next_until_refuted(
        manifest.solver_,
        expected,
        [&]() -> solution {
            return {
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_t))),
            };
        });
}
