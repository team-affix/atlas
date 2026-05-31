// Integration: basic_manifest — wiring, sim lifecycle, cross-tick solver interop.
//
// Harness: snapshot_at_yield, run_one_tick, enumerate_all_solutions, next_until_refuted.
//
// Bug policy (docs/testing.md): failing tests indicate suspected production bugs
// unless setup/lifetime/key definitions are wrong. Do not delete or weaken tests.

#include <functional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>
#include <array>
#include <map>
#include <optional>
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
// Harness structs
// ---------------------------------------------------------------------------

struct TickSnapshot {
    lemma resolution_lemma;
    lemma decision_lemma;
    std::map<uint32_t, const expr*> var_bindings;
};

struct SimTerminationResult {
    sim_termination termination;
    TickSnapshot snapshot;
};

// ---------------------------------------------------------------------------
// Solver run harness
// ---------------------------------------------------------------------------

TickSnapshot snapshot_at_yield(
    basic_manifest& manifest,
    i_normalizer& normalizer,
    expr_pool& saved_expr_pool,
    const std::set<uint32_t>& tracked_vars = {}) {
    lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
        manifest.lineage_pool_.pin(rl);
    lemma decision_lemma = manifest.decision_memory_.derive();

    TickSnapshot snap{std::move(resolution_lemma), std::move(decision_lemma), {}};

    for (uint32_t idx : tracked_vars) {
        const expr* var = manifest.expr_pool_.make(idx);
        const expr* normalized = normalizer.normalize(var);
        snap.var_bindings[idx] = saved_expr_pool.import(normalized);
    }

    return snap;
}

std::optional<SimTerminationResult> run_one_tick(
    basic_manifest& manifest,
    i_normalizer& normalizer,
    expr_pool& saved_expr_pool,
    coroutine<sim_termination, void>& sm,
    const std::set<uint32_t>& tracked_vars = {}) {
    sm.resume();

    if (!sm.has_yield())
        return std::nullopt;

    SimTerminationResult result{sm.consume_yield(),
        snapshot_at_yield(manifest, normalizer, saved_expr_pool, tracked_vars)};
    return result;
}

using solution = std::vector<const expr*>;

void enumerate_all_solutions(
    basic_manifest& manifest,
    i_normalizer& normalizer,
    expr_pool& saved_expr_pool,
    std::set<solution> expected,
    const std::function<solution(const TickSnapshot&)>& get_solution,
    const std::set<uint32_t>& tracked_vars = {}) {
    auto sm = manifest.solver_.solve();
    std::set<solution> visited;
    while (!expected.empty()) {
        solution s;
        do {
            std::optional<SimTerminationResult> tick =
                run_one_tick(manifest, normalizer, saved_expr_pool, sm, tracked_vars);
            ASSERT_TRUE(tick.has_value()) << "solver stopped before all expected solutions found";
            if (tick->termination != sim_termination::solved)
                continue;
            s = get_solution(tick->snapshot);
        } while (visited.count(s));
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
        visited.insert(s);
    }
}

void next_until_refuted(
    basic_manifest& manifest,
    i_normalizer& normalizer,
    expr_pool& saved_expr_pool,
    std::set<solution> expected,
    const std::function<solution(const TickSnapshot&)>& get_solution,
    const std::set<uint32_t>& tracked_vars = {}) {

    
    auto sm = manifest.solver_.solve();
    std::set<solution> visited;
    while (true) {
        std::optional<SimTerminationResult> tick =
            run_one_tick(manifest, normalizer, saved_expr_pool, sm, tracked_vars);
        if (!tick)
            break;
        if (tick->termination != sim_termination::solved)
            continue;
        const solution s = get_solution(tick->snapshot);
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
    std::optional<expr_pool> saved_expr_pool_;
    std::optional<normalizer> normalizer_;

    void SetUp() override {
        bind_saved_expr_pool();
    }

    void bind_saved_expr_pool() {
        saved_loc_.bind_as<i_log_to_current_trail_frame>(saved_trail_);
        saved_expr_pool_.emplace(saved_loc_);
    }

    void bind_normalizer(basic_manifest& manifest) {
        normalizer_.emplace(manifest.loc_);
    }

    const expr* import_saved(const expr& e) {
        return saved_expr_pool_->import(&e);
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

TEST_F(BasicManifestIntegrationTest, TickSnapshotBindingsBeforeTearDown) {
    /*
     * Intent: tick snapshot captures bindings before tear_down; bindings revert on resume.
     * initial goals: f(A, B).
     * rules:
     *   0: f(abc, 123).
     */
    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr head{expr::functor{"f", {&abc, &_123}}};
    database.push(rule{&head, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    bind_normalizer(manifest);
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const uint32_t idx_b = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    const expr* var_b = manifest.expr_pool_.make(idx_b);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a, var_b}));

    const expr* abc_saved = saved_expr_pool_->import(&abc);
    const expr* _123_saved = saved_expr_pool_->import(&_123);

    auto sm = manifest.solver_.solve();
    const std::optional<SimTerminationResult> tick =
        run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm, {idx_a, idx_b});
    ASSERT_TRUE(tick);
    ASSERT_EQ(tick->termination, sim_termination::solved);
    EXPECT_EQ(*tick->snapshot.var_bindings.at(idx_a), *abc_saved);
    EXPECT_EQ(*tick->snapshot.var_bindings.at(idx_b), *_123_saved);

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
    bind_normalizer(manifest);
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
    const std::optional<SimTerminationResult> tick1 =
        run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
    ASSERT_TRUE(tick1);
    ASSERT_EQ(tick1->termination, sim_termination::solved);

    const std::optional<SimTerminationResult> tick2 =
        run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
    ASSERT_TRUE(tick2);
    ASSERT_EQ(tick2->termination, sim_termination::solved);
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
    bind_normalizer(manifest);
    const goal_lineage* gl = manifest.make_initial_goal_lineage_.make(0);
    const resolution_lineage* rl0 =
        manifest.lineage_pool_.make_resolution_lineage(gl, rule_id{0});
    const resolution_lineage* rl1 =
        manifest.lineage_pool_.make_resolution_lineage(gl, rule_id{1});
    manifest.elimination_backlog_.insert_backlogged_elimination(rl0);

    auto sm = manifest.solver_.solve();
    const std::optional<SimTerminationResult> tick =
        run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
    ASSERT_TRUE(tick);
    ASSERT_EQ(tick->termination, sim_termination::solved);
    std::vector<rule_id> resolution_ids;
    for (const resolution_lineage* rl : tick->snapshot.resolution_lemma.get_resolutions())
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
    bind_normalizer(manifest);
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
    const std::optional<SimTerminationResult> tick =
        run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
    ASSERT_TRUE(tick);
    std::vector<rule_id> resolution_ids;
    for (const resolution_lineage* rl : tick->snapshot.resolution_lemma.get_resolutions())
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
    bind_normalizer(manifest);
    auto sm = manifest.solver_.solve();
    const std::optional<SimTerminationResult> tick1 =
        run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
    ASSERT_TRUE(tick1);
    ASSERT_EQ(tick1->termination, sim_termination::solved);
    std::set<rule_id> first_branches;
    for (const resolution_lineage* rl : tick1->snapshot.decision_lemma.get_resolutions())
        if (kBranches.count(rl->idx))
            first_branches.insert(rl->idx);
    for (const resolution_lineage* rl : tick1->snapshot.resolution_lemma.get_resolutions())
        if (kBranches.count(rl->idx))
            first_branches.insert(rl->idx);
    ASSERT_EQ(first_branches.size(), 1u);
    EXPECT_EQ(tick1->snapshot.decision_lemma.get_resolutions().size(), 1u);

    const std::optional<SimTerminationResult> tick2 =
        run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
    ASSERT_TRUE(tick2);
    ASSERT_EQ(tick2->termination, sim_termination::solved);
    EXPECT_TRUE(tick2->snapshot.decision_lemma.get_resolutions().empty());
    std::set<rule_id> second_branches;
    for (const resolution_lineage* rl : tick2->snapshot.decision_lemma.get_resolutions())
        if (kBranches.count(rl->idx))
            second_branches.insert(rl->idx);
    for (const resolution_lineage* rl : tick2->snapshot.resolution_lemma.get_resolutions())
        if (kBranches.count(rl->idx))
            second_branches.insert(rl->idx);
    ASSERT_EQ(second_branches.size(), 1u);
    EXPECT_NE(*first_branches.begin(), *second_branches.begin());
    EXPECT_FALSE(tick1->snapshot.decision_lemma.get_resolutions()
            == tick2->snapshot.decision_lemma.get_resolutions()
        && tick1->snapshot.resolution_lemma.get_resolutions()
            == tick2->snapshot.resolution_lemma.get_resolutions()
        && tick1->snapshot.var_bindings == tick2->snapshot.var_bindings);
}

// Tier S — multi-cycle solver (enumeration)

TEST_F(BasicManifestIntegrationTest, SolverVacuousSolvedOnEmptyProblem) {
    /*
     * Intent: solver yields one vacuous solved tick on an empty problem and completes.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    bind_normalizer(manifest);
    auto sm = manifest.solver_.solve();
    auto tick = run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
    ASSERT_TRUE(tick.has_value());
    EXPECT_EQ(tick->termination, sim_termination::solved);
    EXPECT_TRUE(tick->snapshot.decision_lemma.get_resolutions().empty());
    EXPECT_FALSE(run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm));
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
    bind_normalizer(manifest);
    auto sm = manifest.solver_.solve();
    auto tick = run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
    ASSERT_TRUE(tick.has_value());
    EXPECT_EQ(tick->termination, sim_termination::solved);
    EXPECT_TRUE(tick->snapshot.decision_lemma.get_resolutions().empty());
    EXPECT_FALSE(run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm));
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
    bind_normalizer(manifest);
    auto sm = manifest.solver_.solve();
    auto tick = run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
    ASSERT_TRUE(tick.has_value());
    EXPECT_EQ(tick->termination, sim_termination::conflicted);
    EXPECT_FALSE(run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm));
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
    bind_normalizer(manifest);
    auto sm = manifest.solver_.solve();
    std::vector<TickSnapshot> solutions;
    while (true) {
        std::optional<SimTerminationResult> tick =
            run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
        if (!tick)
            break;
        if (tick->termination == sim_termination::solved)
            solutions.push_back(std::move(tick->snapshot));
    }
    ASSERT_EQ(solutions.size(), 2u);
    std::set<rule_id> seen;
    for (const TickSnapshot& snap : solutions) {
        std::set<rule_id> branches;
        for (const resolution_lineage* rl : snap.decision_lemma.get_resolutions())
            if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                branches.insert(rl->idx);
        for (const resolution_lineage* rl : snap.resolution_lemma.get_resolutions())
            if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                branches.insert(rl->idx);
        ASSERT_EQ(branches.size(), 1u);
        seen.insert(*branches.begin());
    }
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
    bind_normalizer(manifest);
    auto sm = manifest.solver_.solve();
    std::set<rule_id> seen;
    std::set<rule_id> visited;
    while (seen.size() < 2u) {
        rule_id branch;
        do {
            std::optional<SimTerminationResult> tick =
                run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
            ASSERT_TRUE(tick.has_value());
            if (tick->termination != sim_termination::solved)
                continue;
            std::set<rule_id> branches;
            for (const resolution_lineage* rl : tick->snapshot.decision_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    branches.insert(rl->idx);
            for (const resolution_lineage* rl : tick->snapshot.resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    branches.insert(rl->idx);
            ASSERT_EQ(branches.size(), 1u);
            branch = *branches.begin();
        } while (visited.count(branch));
        visited.insert(branch);
        seen.insert(branch);
    }
    EXPECT_THAT(seen, UnorderedElementsAre(rule_id{0}, rule_id{1}));
    std::optional<SimTerminationResult> tick;
    do {
        tick = run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
        if (tick && tick->termination == sim_termination::solved)
            FAIL() << "unexpected extra solution after exhausting search space";
    } while (tick.has_value());
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
    bind_normalizer(manifest);
    auto sm = manifest.solver_.solve();
    auto tick = run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
    ASSERT_TRUE(tick.has_value());
    EXPECT_EQ(tick->termination, sim_termination::solved);
    EXPECT_TRUE(tick->snapshot.decision_lemma.get_resolutions().empty());
    EXPECT_FALSE(run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm));
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
    bind_normalizer(manifest);
    auto sm = manifest.solver_.solve();
    std::vector<TickSnapshot> solutions;
    while (true) {
        std::optional<SimTerminationResult> tick =
            run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
        if (!tick)
            break;
        if (tick->termination == sim_termination::solved)
            solutions.push_back(std::move(tick->snapshot));
    }
    ASSERT_EQ(solutions.size(), 2u);
    std::set<rule_id> seen;
    for (const TickSnapshot& snap : solutions) {
        std::set<rule_id> branches;
        for (const resolution_lineage* rl : snap.decision_lemma.get_resolutions())
            if (rl->idx == rule_id{1} || rl->idx == rule_id{2})
                branches.insert(rl->idx);
        for (const resolution_lineage* rl : snap.resolution_lemma.get_resolutions())
            if (rl->idx == rule_id{1} || rl->idx == rule_id{2})
                branches.insert(rl->idx);
        ASSERT_EQ(branches.size(), 1u);
        seen.insert(*branches.begin());
    }
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
    bind_normalizer(manifest);
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const uint32_t idx_b = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    const expr* var_b = manifest.expr_pool_.make(idx_b);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a, var_b}));

    const expr* abc_saved = saved_expr_pool_->import(&abc);
    const expr* _123_saved = saved_expr_pool_->import(&_123);
    auto sm = manifest.solver_.solve();
    auto tick = run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm, {idx_a, idx_b});
    ASSERT_TRUE(tick.has_value());
    EXPECT_EQ(tick->termination, sim_termination::solved);
    EXPECT_TRUE(tick->snapshot.decision_lemma.get_resolutions().empty());
    EXPECT_EQ(*tick->snapshot.var_bindings.at(idx_a), *abc_saved);
    EXPECT_EQ(*tick->snapshot.var_bindings.at(idx_b), *_123_saved);
    EXPECT_FALSE(run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm, {idx_a, idx_b}));
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
    bind_normalizer(manifest);
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const uint32_t idx_b = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    const expr* var_b = manifest.expr_pool_.make(idx_b);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a, var_b}));

    const expr* abc_saved = saved_expr_pool_->import(&abc);
    const expr* _123_saved = saved_expr_pool_->import(&_123);
    auto sm = manifest.solver_.solve();
    auto tick = run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm, {idx_a, idx_b});
    ASSERT_TRUE(tick.has_value());
    EXPECT_EQ(tick->termination, sim_termination::solved);
    EXPECT_EQ(*tick->snapshot.var_bindings.at(idx_a), *abc_saved);
    EXPECT_EQ(*tick->snapshot.var_bindings.at(idx_b), *_123_saved);
    EXPECT_FALSE(run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm, {idx_a, idx_b}));
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
    bind_normalizer(manifest);
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a}));

    const expr* abc_saved = import_saved(abc);
    const expr* xyz_saved = import_saved(xyz);
    enumerate_all_solutions(
        manifest, *normalizer_, *saved_expr_pool_,
        {{abc_saved}, {xyz_saved}},
        [idx_a](const TickSnapshot& snap) -> solution {
            return {snap.var_bindings.at(idx_a)};
        },
        {idx_a});
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
    bind_normalizer(manifest);
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a}));

    const expr* abc_saved = import_saved(abc);
    const expr* xyz_saved = import_saved(xyz);
    next_until_refuted(
        manifest, *normalizer_, *saved_expr_pool_,
        {{abc_saved}, {xyz_saved}},
        [idx_a](const TickSnapshot& snap) -> solution {
            return {snap.var_bindings.at(idx_a)};
        },
        {idx_a});
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
    bind_normalizer(manifest);
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a}));
    initial_goals.push(manifest.expr_pool_.make("g", {var_a}));

    const expr* abc_saved = import_saved(abc);
    const expr* xyz_saved = import_saved(xyz);
    enumerate_all_solutions(
        manifest, *normalizer_, *saved_expr_pool_,
        {{abc_saved}, {xyz_saved}},
        [idx_a](const TickSnapshot& snap) -> solution {
            return {snap.var_bindings.at(idx_a)};
        },
        {idx_a});
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
    bind_normalizer(manifest);
    auto sm = manifest.solver_.solve();
    std::set<rule_id> seen;
    std::set<rule_id> visited;
    while (seen.size() < 3u) {
        rule_id branch;
        do {
            std::optional<SimTerminationResult> tick =
                run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
            ASSERT_TRUE(tick.has_value());
            if (tick->termination != sim_termination::solved)
                continue;
            std::set<rule_id> branches;
            for (const resolution_lineage* rl : tick->snapshot.decision_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1} || rl->idx == rule_id{2})
                    branches.insert(rl->idx);
            for (const resolution_lineage* rl : tick->snapshot.resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1} || rl->idx == rule_id{2})
                    branches.insert(rl->idx);
            ASSERT_EQ(branches.size(), 1u);
            branch = *branches.begin();
        } while (visited.count(branch));
        visited.insert(branch);
        seen.insert(branch);
    }
    EXPECT_THAT(seen, UnorderedElementsAre(rule_id{0}, rule_id{1}, rule_id{2}));
    std::optional<SimTerminationResult> tick;
    do {
        tick = run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
        if (tick && tick->termination == sim_termination::solved)
            FAIL() << "unexpected extra solution after exhausting search space";
    } while (tick.has_value());
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
    bind_normalizer(manifest);
    auto sm = manifest.solver_.solve();
    std::set<std::pair<rule_id, rule_id>> seen;
    std::set<std::pair<rule_id, rule_id>> visited;
    while (seen.size() < 4u) {
        std::pair<rule_id, rule_id> pair;
        do {
            std::optional<SimTerminationResult> tick =
                run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
            ASSERT_TRUE(tick.has_value());
            if (tick->termination != sim_termination::solved)
                continue;
            std::set<rule_id> f_branch;
            for (const resolution_lineage* rl : tick->snapshot.decision_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    f_branch.insert(rl->idx);
            for (const resolution_lineage* rl : tick->snapshot.resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    f_branch.insert(rl->idx);
            std::set<rule_id> g_branch;
            for (const resolution_lineage* rl : tick->snapshot.decision_lemma.get_resolutions())
                if (rl->idx == rule_id{2} || rl->idx == rule_id{3})
                    g_branch.insert(rl->idx);
            for (const resolution_lineage* rl : tick->snapshot.resolution_lemma.get_resolutions())
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
    std::optional<SimTerminationResult> tick;
    do {
        tick = run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
        if (tick && tick->termination == sim_termination::solved)
            FAIL() << "unexpected extra solution after exhausting search space";
    } while (tick.has_value());
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
        std::vector<TickSnapshot> solutions;
        while (true) {
            std::optional<SimTerminationResult> tick =
                run_one_tick(manifest, seed_normalizer, seed_pool, sm);
            if (!tick)
                break;
            if (tick->termination == sim_termination::solved)
                solutions.push_back(std::move(tick->snapshot));
        }
        ASSERT_EQ(solutions.size(), 8u);
        std::set<std::tuple<rule_id, rule_id, rule_id>> seen;
        for (const TickSnapshot& snap : solutions) {
            std::set<rule_id> f_branch;
            for (const resolution_lineage* rl : snap.decision_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    f_branch.insert(rl->idx);
            for (const resolution_lineage* rl : snap.resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    f_branch.insert(rl->idx);
            std::set<rule_id> g_branch;
            for (const resolution_lineage* rl : snap.decision_lemma.get_resolutions())
                if (rl->idx == rule_id{2} || rl->idx == rule_id{3})
                    g_branch.insert(rl->idx);
            for (const resolution_lineage* rl : snap.resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{2} || rl->idx == rule_id{3})
                    g_branch.insert(rl->idx);
            std::set<rule_id> h_branch;
            for (const resolution_lineage* rl : snap.decision_lemma.get_resolutions())
                if (rl->idx == rule_id{4} || rl->idx == rule_id{5})
                    h_branch.insert(rl->idx);
            for (const resolution_lineage* rl : snap.resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{4} || rl->idx == rule_id{5})
                    h_branch.insert(rl->idx);
            ASSERT_EQ(f_branch.size(), 1u);
            ASSERT_EQ(g_branch.size(), 1u);
            ASSERT_EQ(h_branch.size(), 1u);
            seen.insert({*f_branch.begin(), *g_branch.begin(), *h_branch.begin()});
        }
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
    bind_normalizer(manifest);
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a}));

    const expr* abc_saved = import_saved(abc);
    const expr* xyz_saved = import_saved(xyz);
    const expr* def_saved = import_saved(def);
    const expr* ghi_saved = import_saved(ghi);
    next_until_refuted(
        manifest, *normalizer_, *saved_expr_pool_,
        {{abc_saved}, {xyz_saved}, {def_saved}, {ghi_saved}},
        [idx_a](const TickSnapshot& snap) -> solution {
            return {snap.var_bindings.at(idx_a)};
        },
        {idx_a});
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
    bind_normalizer(manifest);
    auto sm = manifest.solver_.solve();
    std::set<rule_id> seen;
    std::set<rule_id> visited;
    while (seen.size() < 4u) {
        rule_id fact;
        do {
            std::optional<SimTerminationResult> tick =
                run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
            ASSERT_TRUE(tick.has_value());
            if (tick->termination != sim_termination::solved)
                continue;
            std::set<rule_id> g_fact;
            for (const resolution_lineage* rl : tick->snapshot.decision_lemma.get_resolutions())
                if (rl->idx == rule_id{1} || rl->idx == rule_id{2} || rl->idx == rule_id{3}
                    || rl->idx == rule_id{4})
                    g_fact.insert(rl->idx);
            for (const resolution_lineage* rl : tick->snapshot.resolution_lemma.get_resolutions())
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
    std::optional<SimTerminationResult> tick;
    do {
        tick = run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
        if (tick && tick->termination == sim_termination::solved)
            FAIL() << "unexpected extra solution after exhausting search space";
    } while (tick.has_value());
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

        const std::set<uint32_t> tracked{idx_a, idx_b, idx_c};
        auto sm = manifest.solver_.solve();
        std::vector<TickSnapshot> solutions;
        while (true) {
            std::optional<SimTerminationResult> tick =
                run_one_tick(manifest, seed_normalizer, seed_pool, sm, tracked);
            if (!tick)
                break;
            if (tick->termination == sim_termination::solved)
                solutions.push_back(std::move(tick->snapshot));
        }
        ASSERT_EQ(solutions.size(), kRawSolutions);
        std::set<std::pair<rule_id, rule_id>> seen;
        for (const TickSnapshot& snap : solutions) {
            std::set<rule_id> f_branch;
            for (const resolution_lineage* rl : snap.decision_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    f_branch.insert(rl->idx);
            for (const resolution_lineage* rl : snap.resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{0} || rl->idx == rule_id{1})
                    f_branch.insert(rl->idx);
            std::set<rule_id> g_branch;
            for (const resolution_lineage* rl : snap.decision_lemma.get_resolutions())
                if (rl->idx == rule_id{2} || rl->idx == rule_id{3} || rl->idx == rule_id{4}
                    || rl->idx == rule_id{5} || rl->idx == rule_id{6})
                    g_branch.insert(rl->idx);
            for (const resolution_lineage* rl : snap.resolution_lemma.get_resolutions())
                if (rl->idx == rule_id{2} || rl->idx == rule_id{3} || rl->idx == rule_id{4}
                    || rl->idx == rule_id{5} || rl->idx == rule_id{6})
                    g_branch.insert(rl->idx);
            ASSERT_EQ(f_branch.size(), 1u);
            ASSERT_EQ(g_branch.size(), 1u);
            seen.insert({*f_branch.begin(), *g_branch.begin()});
        }
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
        for (const TickSnapshot& snap : solutions) {
            ASSERT_EQ(snap.var_bindings.size(), 3u);
            const expr& a_bound = *snap.var_bindings.at(idx_a);
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
            EXPECT_EQ(*snap.var_bindings.at(idx_b), *xyz_saved);
            EXPECT_EQ(*snap.var_bindings.at(idx_c), *pqr_saved);
        }
        EXPECT_EQ(saw_abc, 2u);
        EXPECT_EQ(saw_def, 2u);
        EXPECT_EQ(saw_ghi, 2u);
        EXPECT_EQ(saw_jkl, 2u);
        EXPECT_EQ(saw_mno, 2u);
    }
}
