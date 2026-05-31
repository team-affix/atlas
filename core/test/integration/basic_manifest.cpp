// Integration: basic_manifest as a whole session object — wiring, sim lifecycle,
// cross-tick solver interop, and multi-cycle search semantics.
//
// Harness API (general N-way; no cardinality-named helpers):
//   Run: run_solver, run_one_tick, snapshot_at_yield
//   Branch: expect_branch_enumeration, expect_solver_branch_enumeration (enum + exhaustion)
//   Binding: expect_binding_enumeration (enum-only size + match), expect_all_solutions_by_bindings
//   Exhaustion: expect_enumeration_complete — pair separately for enum-only tier-S tests
//
// Bug policy (docs/testing.md): failing tests indicate suspected production bugs
// unless setup/lifetime/key definitions are wrong. Do not delete or weaken tests.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <set>
#include <string>
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
    std::set<rule_id> decision_rule_ids;
    std::set<rule_id> resolution_rule_ids;
    std::map<uint32_t, const expr*> var_bindings;
};

struct SimTerminationResult {
    sim_termination termination{};
    TickSnapshot snapshot;
};

struct SolverRun {
    std::vector<sim_termination> terminations;
    std::vector<TickSnapshot> solutions;
    bool completed{};
};

// ---------------------------------------------------------------------------
// Primitives
// ---------------------------------------------------------------------------

using BindingKey = std::map<uint32_t, const expr*>;

bool binding_equal(const BindingKey& a, const BindingKey& b) {
    if (a.size() != b.size())
        return false;
    for (const auto& [idx, ea] : a) {
        auto it = b.find(idx);
        if (it == b.end() || *ea != *it->second)
            return false;
    }
    return true;
}

bool snapshot_equal(const TickSnapshot& a, const TickSnapshot& b) {
    if (a.decision_rule_ids != b.decision_rule_ids)
        return false;
    if (a.resolution_rule_ids != b.resolution_rule_ids)
        return false;
    return binding_equal(a.var_bindings, b.var_bindings);
}

std::set<rule_id> branch_ids(
    const TickSnapshot& snap,
    const std::set<rule_id>& candidate_rules) {
    std::set<rule_id> ids;
    for (rule_id id : snap.decision_rule_ids) {
        if (candidate_rules.count(id))
            ids.insert(id);
    }
    for (rule_id id : snap.resolution_rule_ids) {
        if (candidate_rules.count(id))
            ids.insert(id);
    }
    return ids;
}

// ---------------------------------------------------------------------------
// Solver run harness
// ---------------------------------------------------------------------------

TickSnapshot snapshot_at_yield(
    basic_manifest& manifest,
    i_normalizer& normalizer,
    expr_pool& saved_expr_pool,
    const std::set<uint32_t>& tracked_vars = {}) {
    TickSnapshot snap;

    const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions()) {
        manifest.lineage_pool_.pin(rl);
        snap.resolution_rule_ids.insert(rl->idx);
    }

    const lemma decision_lemma = manifest.decision_memory_.derive();
    for (const resolution_lineage* rl : decision_lemma.get_resolutions())
        snap.decision_rule_ids.insert(rl->idx);

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

    SimTerminationResult result;
    result.termination = sm.consume_yield();
    result.snapshot = snapshot_at_yield(manifest, normalizer, saved_expr_pool, tracked_vars);
    return result;
}

SolverRun run_solver(
    basic_manifest& manifest,
    i_normalizer& normalizer,
    expr_pool& saved_expr_pool,
    const std::set<uint32_t>& tracked_vars = {}) {
    SolverRun run;
    auto sm = manifest.solver_.solve();
    while (!sm.done()) {
        std::optional<SimTerminationResult> tick =
            run_one_tick(manifest, normalizer, saved_expr_pool, sm, tracked_vars);
        if (!tick)
            break;
        run.terminations.push_back(tick->termination);
        if (tick->termination == sim_termination::solved)
            run.solutions.push_back(tick->snapshot);
    }
    run.completed = sm.done();
    return run;
}

using BranchGroups = std::vector<std::set<rule_id>>;

namespace branch_enum_detail {

size_t branch_group_product(const BranchGroups& groups) {
    size_t count = 1;
    for (const std::set<rule_id>& group : groups) {
        if (group.empty())
            return 0;
        count *= group.size();
    }
    return count;
}

std::vector<std::vector<rule_id>> branch_cartesian_product(const BranchGroups& groups) {
    std::vector<std::vector<rule_id>> tuples;
    if (groups.empty())
        return tuples;
    tuples.push_back({});
    for (const std::set<rule_id>& group : groups) {
        std::vector<std::vector<rule_id>> next;
        for (const std::vector<rule_id>& prefix : tuples) {
            for (rule_id id : group) {
                std::vector<rule_id> extended = prefix;
                extended.push_back(id);
                next.push_back(std::move(extended));
            }
        }
        tuples = std::move(next);
    }
    return tuples;
}

std::vector<rule_id> branch_tuple_for_snapshot(
    const TickSnapshot& snap,
    const BranchGroups& groups) {
    std::vector<rule_id> tuple;
    tuple.reserve(groups.size());
    for (const std::set<rule_id>& group : groups) {
        const std::set<rule_id> branches = branch_ids(snap, group);
        if (branches.size() != 1)
            return {};
        tuple.push_back(*branches.begin());
    }
    return tuple;
}

}  // namespace branch_enum_detail

// ---------------------------------------------------------------------------
// Branch enumeration
// ---------------------------------------------------------------------------

void expect_branch_enumeration(
    const std::vector<TickSnapshot>& solutions,
    const BranchGroups& groups) {
    // groups must be disjoint — overlapping rule ids can yield >1 branch per group
    const size_t expected_count = branch_enum_detail::branch_group_product(groups);
    ASSERT_EQ(solutions.size(), expected_count);

    const std::vector<std::vector<rule_id>> expected_tuples =
        branch_enum_detail::branch_cartesian_product(groups);
    std::vector<std::vector<rule_id>> seen;
    for (const TickSnapshot& snap : solutions) {
        const std::vector<rule_id> tuple =
            branch_enum_detail::branch_tuple_for_snapshot(snap, groups);
        ASSERT_EQ(tuple.size(), groups.size()) << "each solution should identify one branch per group";
        seen.push_back(tuple);
    }

    for (size_t i = 0; i < seen.size(); ++i) {
        for (size_t j = i + 1; j < seen.size(); ++j)
            EXPECT_NE(seen[i], seen[j]) << "duplicate branch tuple";
    }

    for (const std::vector<rule_id>& expected : expected_tuples) {
        EXPECT_TRUE(std::ranges::find(seen, expected) != seen.end())
            << "missing expected branch tuple";
    }
}

// ---------------------------------------------------------------------------
// Binding enumeration and exhaustion
// ---------------------------------------------------------------------------

void expect_all_solutions_by_bindings(
    const std::vector<TickSnapshot>& solutions,
    std::vector<BindingKey> expected) {
    // order of expected bindings is irrelevant
    std::vector<BindingKey> collapsed;
    for (const TickSnapshot& snap : solutions) {
        if (std::ranges::any_of(collapsed,
                [&](const BindingKey& key) { return binding_equal(key, snap.var_bindings); }))
            continue;
        collapsed.push_back(snap.var_bindings);
    }
    ASSERT_EQ(collapsed.size(), expected.size());

    for (const BindingKey& actual : collapsed) {
        auto it = std::ranges::find_if(expected,
            [&](const BindingKey& key) { return binding_equal(key, actual); });
        ASSERT_NE(it, expected.end()) << "unexpected binding key";
        expected.erase(it);
    }
    EXPECT_TRUE(expected.empty()) << "unmatched expected binding keys remain";
}

void expect_enumeration_complete(const SolverRun& run, size_t expected_solutions) {
    ASSERT_EQ(run.solutions.size(), expected_solutions);
    EXPECT_EQ(std::ranges::count(run.terminations, sim_termination::solved), expected_solutions);
    EXPECT_TRUE(run.completed);
}

// ---------------------------------------------------------------------------
// Composable solver checks
// ---------------------------------------------------------------------------

void expect_binding_enumeration(
    const std::vector<TickSnapshot>& solutions,
    const std::vector<BindingKey>& expected) {
    // enumeration-only: raw size + collapsed binding match (not full snapshot distinctness)
    ASSERT_EQ(solutions.size(), expected.size());
    expect_all_solutions_by_bindings(solutions, expected);
}

void expect_one_branch_per_group(const TickSnapshot& snap, const BranchGroups& groups) {
    // groups must be disjoint — same invariant as expect_branch_enumeration
    for (const std::set<rule_id>& group : groups)
        ASSERT_EQ(branch_ids(snap, group).size(), 1u);
}

void expect_solver_branch_enumeration(const SolverRun& run, const BranchGroups& groups) {
    expect_branch_enumeration(run.solutions, groups);
    expect_enumeration_complete(run, branch_enum_detail::branch_group_product(groups));
}

template<typename Iface, typename Concrete>
void expect_same_instance(Concrete& concrete, locator& loc) {
    EXPECT_EQ(static_cast<Iface*>(&concrete), &loc.locate<Iface>());
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

    BindingKey binding_key(uint32_t idx, const expr& e) {
        return {{idx, import_saved(e)}};
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
    expect_same_instance<i_learn_avoidance>(manifest.cdcl_, manifest.loc_);
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
    expect_same_instance<i_elimination_generator>(manifest.joint_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringTrailSharedForPushPopLog) {
    /*
     * Intent: trail_ backs i_push_trail_frame, i_pop_trail_frame, and i_log_to_current_trail_frame.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_push_trail_frame>(manifest.trail_, manifest.loc_);
    expect_same_instance<i_pop_trail_frame>(manifest.trail_, manifest.loc_);
    expect_same_instance<i_log_to_current_trail_frame>(manifest.trail_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringDecisionMemorySharedForRecordCountDerive) {
    /*
     * Intent: decision_memory_ backs record/clear/count/derive decision interfaces.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_record_decision>(manifest.decision_memory_, manifest.loc_);
    expect_same_instance<i_clear_recorded_decisions>(manifest.decision_memory_, manifest.loc_);
    expect_same_instance<i_get_decision_count>(manifest.decision_memory_, manifest.loc_);
    expect_same_instance<i_derive_decision_lemma>(manifest.decision_memory_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringSimSharedForSetUpRunTearDown) {
    /*
     * Intent: sim_ backs i_set_up_sim, i_run_sim, and i_tear_down_sim.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_set_up_sim>(manifest.sim_, manifest.loc_);
    expect_same_instance<i_tear_down_sim>(manifest.sim_, manifest.loc_);
    expect_same_instance<i_run_sim>(manifest.sim_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringMhuSharedForHeadOps) {
    /*
     * Intent: mhu_ backs i_try_add_mhu_head and i_clear_mhu_heads.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_try_add_mhu_head>(manifest.mhu_, manifest.loc_);
    expect_same_instance<i_clear_mhu_heads>(manifest.mhu_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringLineagePoolSharedForMakePinTrim) {
    /*
     * Intent: lineage_pool_ backs make/pin/trim lineage interfaces.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_make_goal_lineage>(manifest.lineage_pool_, manifest.loc_);
    expect_same_instance<i_make_resolution_lineage>(manifest.lineage_pool_, manifest.loc_);
    expect_same_instance<i_pin_goal_lineage>(manifest.lineage_pool_, manifest.loc_);
    expect_same_instance<i_pin_resolution_lineage>(manifest.lineage_pool_, manifest.loc_);
    expect_same_instance<i_trim_unpinned_lineages>(manifest.lineage_pool_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringSolverIsISolve) {
    /*
     * Intent: solver_ is the locator's i_solve binding.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_solve>(manifest.solver_, manifest.loc_);
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
    expect_same_instance<i_record_resolution>(manifest.resolution_memory_, manifest.loc_);
    expect_same_instance<i_clear_recorded_resolutions>(manifest.resolution_memory_, manifest.loc_);
    expect_same_instance<i_derive_resolution_lemma>(manifest.resolution_memory_, manifest.loc_);
    expect_same_instance<i_get_resolution_count>(manifest.resolution_memory_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringBindMapSharedForWhnfAndClear) {
    /*
     * Intent: bind_map_ backs i_bind_map and i_clear_bindings.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_bind_map>(manifest.bind_map_, manifest.loc_);
    expect_same_instance<i_clear_bindings>(manifest.bind_map_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringResolverAndRouterBound) {
    /*
     * Intent: resolver_ and elimination_router_ are bound on the locator.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_resolver>(manifest.resolver_, manifest.loc_);
    expect_same_instance<i_elimination_router>(manifest.elimination_router_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringRandomDecisionGeneratorIsGenerateDecision) {
    /*
     * Intent: random_decision_generator_ is the locator's i_generate_decision binding.
     * initial goals: (none)
     * rules: (none)
     */
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_generate_decision>(manifest.random_decision_generator_, manifest.loc_);
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
    expect_same_instance<i_bind_map>(manifest.bind_map_, manifest.loc_);
    expect_same_instance<i_bind_map_factory>(manifest.bind_map_factory_, manifest.loc_);
    expect_same_instance<i_make_functor>(manifest.expr_pool_, manifest.loc_);
    expect_same_instance<i_import_expr>(manifest.expr_pool_, manifest.loc_);
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
    EXPECT_THAT(tick->snapshot.resolution_rule_ids, UnorderedElementsAre(rule_id{1}));
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
    EXPECT_THAT(tick->snapshot.resolution_rule_ids,
        UnorderedElementsAre(rule_id{0}, rule_id{3}));
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
    const std::set<rule_id> first_branches = branch_ids(tick1->snapshot, kBranches);
    ASSERT_EQ(first_branches.size(), 1u);
    EXPECT_EQ(tick1->snapshot.decision_rule_ids.size(), 1u);

    const std::optional<SimTerminationResult> tick2 =
        run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
    ASSERT_TRUE(tick2);
    ASSERT_EQ(tick2->termination, sim_termination::solved);
    EXPECT_TRUE(tick2->snapshot.decision_rule_ids.empty());
    const std::set<rule_id> second_branches = branch_ids(tick2->snapshot, kBranches);
    ASSERT_EQ(second_branches.size(), 1u);
    EXPECT_NE(*first_branches.begin(), *second_branches.begin());
    EXPECT_FALSE(snapshot_equal(tick1->snapshot, tick2->snapshot));
    EXPECT_FALSE(tick1->snapshot.decision_rule_ids == tick2->snapshot.decision_rule_ids
        && tick1->snapshot.resolution_rule_ids == tick2->snapshot.resolution_rule_ids);
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
    const SolverRun run = run_solver(manifest, *normalizer_, *saved_expr_pool_);
    ASSERT_EQ(run.terminations.size(), 1u);
    EXPECT_EQ(run.terminations.front(), sim_termination::solved);
    ASSERT_EQ(run.solutions.size(), 1u);
    EXPECT_TRUE(run.solutions.front().decision_rule_ids.empty());
    EXPECT_TRUE(run.completed);
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
    const SolverRun run = run_solver(manifest, *normalizer_, *saved_expr_pool_);
    ASSERT_EQ(run.solutions.size(), 1u);
    EXPECT_TRUE(run.solutions.front().decision_rule_ids.empty());
    EXPECT_TRUE(run.completed);
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
    const SolverRun run = run_solver(manifest, *normalizer_, *saved_expr_pool_);
    EXPECT_TRUE(run.solutions.empty());
    ASSERT_EQ(run.terminations.size(), 1u);
    EXPECT_EQ(run.terminations.front(), sim_termination::conflicted);
    EXPECT_TRUE(run.completed);
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
    const SolverRun run = run_solver(manifest, *normalizer_, *saved_expr_pool_);
    expect_branch_enumeration(run.solutions, {{rule_id{0}, rule_id{1}}});
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
    const SolverRun run = run_solver(manifest, *normalizer_, *saved_expr_pool_);
    expect_solver_branch_enumeration(run, {{rule_id{0}, rule_id{1}}});
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
    const SolverRun run = run_solver(manifest, *normalizer_, *saved_expr_pool_);
    ASSERT_EQ(run.solutions.size(), 1u);
    EXPECT_TRUE(run.solutions.front().decision_rule_ids.empty());
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
    const SolverRun run = run_solver(manifest, *normalizer_, *saved_expr_pool_);
    expect_branch_enumeration(run.solutions, {{rule_id{1}, rule_id{2}}});
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
    const SolverRun run =
        run_solver(manifest, *normalizer_, *saved_expr_pool_, {idx_a, idx_b});
    ASSERT_EQ(run.solutions.size(), 1u);
    EXPECT_TRUE(run.solutions.front().decision_rule_ids.empty());
    EXPECT_EQ(*run.solutions.front().var_bindings.at(idx_a), *abc_saved);
    EXPECT_EQ(*run.solutions.front().var_bindings.at(idx_b), *_123_saved);
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
    const SolverRun run =
        run_solver(manifest, *normalizer_, *saved_expr_pool_, {idx_a, idx_b});
    ASSERT_EQ(run.solutions.size(), 1u);
    EXPECT_EQ(*run.solutions.front().var_bindings.at(idx_a), *abc_saved);
    EXPECT_EQ(*run.solutions.front().var_bindings.at(idx_b), *_123_saved);
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

    const SolverRun run =
        run_solver(manifest, *normalizer_, *saved_expr_pool_, {idx_a});
    expect_binding_enumeration(run.solutions, {
        binding_key(idx_a, abc),
        binding_key(idx_a, xyz),
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
    bind_normalizer(manifest);
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a}));

    const SolverRun run =
        run_solver(manifest, *normalizer_, *saved_expr_pool_, {idx_a});
    expect_all_solutions_by_bindings(run.solutions, {
        binding_key(idx_a, abc),
        binding_key(idx_a, xyz),
    });
    expect_enumeration_complete(run, 2);
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

    const SolverRun run =
        run_solver(manifest, *normalizer_, *saved_expr_pool_, {idx_a});
    expect_binding_enumeration(run.solutions, {
        binding_key(idx_a, abc),
        binding_key(idx_a, xyz),
    });
}

// Cross-tick enumeration end-to-end (uses same helpers as S-tier).

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
    const SolverRun run = run_solver(manifest, *normalizer_, *saved_expr_pool_);
    expect_solver_branch_enumeration(run, {{rule_id{0}, rule_id{1}, rule_id{2}}});
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

    TickSnapshot snap;
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
        snap.resolution_rule_ids.insert(rl->idx);
    expect_one_branch_per_group(snap, {{rule_id{0}, rule_id{1}}, {rule_id{2}, rule_id{3}}});
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

    TickSnapshot snap;
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
        snap.resolution_rule_ids.insert(rl->idx);
    expect_one_branch_per_group(snap, {{rule_id{0}, rule_id{1}}, {rule_id{2}, rule_id{3}}});
    // CDCL forbids g/2 once f is committed; when RNG picks f before g (one decision),
    // g/3 is unit-forced. Two decisions occur if g is chosen first — still valid.
    if (manifest.decision_memory_.count() == 1u)
        EXPECT_FALSE(branch_ids(snap, {rule_id{2}, rule_id{3}}).contains(rule_id{2}));
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
    const SolverRun run = run_solver(manifest, *normalizer_, *saved_expr_pool_);
    expect_solver_branch_enumeration(run, {{rule_id{0}, rule_id{1}}, {rule_id{2}, rule_id{3}}});
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

        const SolverRun run = run_solver(manifest, seed_normalizer, seed_pool);
        expect_solver_branch_enumeration(run,
            {{rule_id{0}, rule_id{1}}, {rule_id{2}, rule_id{3}}, {rule_id{4}, rule_id{5}}});
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

    const SolverRun run =
        run_solver(manifest, *normalizer_, *saved_expr_pool_, {idx_a});
    expect_all_solutions_by_bindings(run.solutions, {
        binding_key(idx_a, abc),
        binding_key(idx_a, xyz),
        binding_key(idx_a, def),
        binding_key(idx_a, ghi),
    });
    expect_enumeration_complete(run, 4);
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
    const SolverRun run = run_solver(manifest, *normalizer_, *saved_expr_pool_);
    expect_solver_branch_enumeration(run, {{rule_id{1}, rule_id{2}, rule_id{3}, rule_id{4}}});
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

        const SolverRun run =
            run_solver(manifest, seed_normalizer, seed_pool, {idx_a, idx_b, idx_c});
        expect_branch_enumeration(run.solutions,
            {{rule_id{0}, rule_id{1}}, {rule_id{2}, rule_id{3}, rule_id{4}, rule_id{5}, rule_id{6}}});
        expect_all_solutions_by_bindings(run.solutions, {
            BindingKey{{idx_a, seed_pool.import(&abc)}, {idx_b, seed_pool.import(&xyz)}, {idx_c, seed_pool.import(&pqr)}},
            BindingKey{{idx_a, seed_pool.import(&def)}, {idx_b, seed_pool.import(&xyz)}, {idx_c, seed_pool.import(&pqr)}},
            BindingKey{{idx_a, seed_pool.import(&ghi)}, {idx_b, seed_pool.import(&xyz)}, {idx_c, seed_pool.import(&pqr)}},
            BindingKey{{idx_a, seed_pool.import(&jkl)}, {idx_b, seed_pool.import(&xyz)}, {idx_c, seed_pool.import(&pqr)}},
            BindingKey{{idx_a, seed_pool.import(&mno)}, {idx_b, seed_pool.import(&xyz)}, {idx_c, seed_pool.import(&pqr)}},
        });
        expect_enumeration_complete(run, kRawSolutions);
    }
}
