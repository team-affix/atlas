// Integration: basic_manifest — wiring, sim lifecycle, cross-tick solver interop.
//
// Harness:
//   enumerate_all_solutions, next_until_refuted — binding enumeration
//   next_branch_until_refuted — resolution-branch enumeration until refutation;
//     asserts no duplicate branch visitation (stricter than next_until_refuted)
// Solver ticks: sm.resume / has_yield / consume_yield.
//
// Bug policy (docs/testing.md): failing tests indicate suspected production bugs
// unless setup/lifetime/key definitions are wrong. Do not delete or weaken tests.

#include <functional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "infrastructure/basic_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/locator.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/var_names.hpp"
#include "interfaces/i_expr_printer.hpp"
#include "interfaces/i_var_names.hpp"
#include "interfaces/i_pop_trail_frame.hpp"
#include "interfaces/i_push_trail_frame.hpp"
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
// Binding enumeration harness
// ---------------------------------------------------------------------------

using solution = std::vector<const expr*>;

void print_solution(
    size_t solution_index,
    i_expr_printer& printer,
    size_t iterations_since_last,
    i_get_decision_count& decision_count,
    i_get_resolution_count& resolution_count,
    const solution& s) {
    std::cout << "solution " << solution_index << ", " << iterations_since_last << " iterations, "
              << resolution_count.get_resolution_count() << " resolutions, "
              << decision_count.count() << " decisions\n";

    for (const expr* e : s) {
        std::cout << "  ";
        printer.print(e);
        std::cout << '\n';
    }
}

void enumerate_all_solutions(
    i_solve& solver,
    i_expr_printer& printer,
    i_get_decision_count& decision_count,
    i_get_resolution_count& resolution_count,
    std::set<solution> expected,
    const std::function<solution()>& get_solution) {
    auto sm = solver.solve();
    std::set<solution> visited;
    size_t iterations_since_last = 0;
    size_t solution_index = 0;
    while (!expected.empty()) {
        solution s;
        do {
            sm.resume();
            ++iterations_since_last;
            ASSERT_TRUE(sm.has_yield()) << "solver stopped before all expected solutions found";
            if (sm.consume_yield() != sim_termination::solved)
                continue;
            s = get_solution();
            print_solution(
                solution_index++,
                printer,
                iterations_since_last,
                decision_count,
                resolution_count,
                s);
            iterations_since_last = 0;
        } while (visited.count(s));
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
        visited.insert(s);
    }
}

void next_until_refuted(
    i_solve& solver,
    i_expr_printer& printer,
    i_get_decision_count& decision_count,
    i_get_resolution_count& resolution_count,
    std::set<solution> expected,
    const std::function<solution()>& get_solution) {
    auto sm = solver.solve();
    std::set<solution> visited;
    size_t iterations_since_last = 0;
    size_t solution_index = 0;
    while (true) {
        sm.resume();
        ++iterations_since_last;
        if (!sm.has_yield())
            break;
        if (sm.consume_yield() != sim_termination::solved)
            continue;
        const solution s = get_solution();
        print_solution(
            solution_index++,
            printer,
            iterations_since_last,
            decision_count,
            resolution_count,
            s);
        iterations_since_last = 0;
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver refuted before all expected solutions found";
}

// ---------------------------------------------------------------------------
// Resolution branch enumeration harness
// ---------------------------------------------------------------------------

using BranchGroups = std::vector<std::set<rule_id>>;
using BranchTuple = std::vector<rule_id>;

void next_branch_until_refuted(
    i_solve& solver,
    i_derive_resolution_lemma& resolution_memory,
    const BranchGroups& groups,
    std::set<BranchTuple> expected) {
    auto sm = solver.solve();
    std::set<BranchTuple> visited;
    while (true) {
        sm.resume();
        if (!sm.has_yield())
            break;
        if (sm.consume_yield() != sim_termination::solved)
            continue;

        const lemma resolution_lemma = resolution_memory.derive_resolution_lemma();
        BranchTuple tuple;
        tuple.reserve(groups.size());
        for (const std::set<rule_id>& group : groups) {
            std::set<rule_id> found;
            for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
                if (group.count(rl->idx))
                    found.insert(rl->idx);
            ASSERT_EQ(found.size(), 1u);
            tuple.push_back(*found.begin());
        }

        ASSERT_FALSE(visited.count(tuple)) << "resolution branch enumerated twice";
        visited.insert(tuple);

        auto it = expected.find(tuple);
        ASSERT_NE(it, expected.end()) << "unexpected resolution branch";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver refuted before all expected resolution branches found";
}

}  // namespace

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

struct expr_printer_context {
    struct var_names_adapter : i_var_names {
        var_names& inner;

        explicit var_names_adapter(var_names& inner) : inner(inner) {}

        bool is_named(uint32_t index) const override { return inner.is_named(index); }
        const std::string& name(uint32_t index) const override { return inner.name(index); }
        void set_name(uint32_t index, const std::string& name) override { inner.set_name(index, name); }
    };

    var_names_adapter adapter;
    locator loc;
    expr_printer printer;

    explicit expr_printer_context(var_names& var_names)
        : adapter(var_names),
          loc(),
          printer(std::cout, bind_loc(loc, adapter)) {}

private:
    static locator& bind_loc(locator& loc, var_names_adapter& adapter) {
        loc.bind_as<i_var_names>(adapter);
        return loc;
    }
};

struct BasicManifestIntegrationTest : public ::testing::Test {
    static constexpr size_t kMaxResolutions = 32;
    static constexpr uint32_t kSeed = 42;

    db database;
    initial_goal_exprs initial_goals;

    trail saved_trail_;
    locator saved_loc_;
    expr_pool saved_expr_pool_{bind_saved_loc(saved_trail_, saved_loc_)};
    var_names var_names_;
    expr_printer_context expr_printer_{var_names_};

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
    const expr* goal = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
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
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* f_head = saved_expr_pool_.make("f", {});
    const expr* f_body = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
    database.push(rule{f_head, {f_body}});
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
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* _123 = saved_expr_pool_.make("123", {});
    const expr* head = saved_expr_pool_.make("f", {abc, _123});
    database.push(rule{head, {}});

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
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* _123 = saved_expr_pool_.make("123", {});
    const expr* head = saved_expr_pool_.make("f", {abc, _123});
    database.push(rule{head, {}});

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
    const expr* rule_ignored = saved_expr_pool_.make(0);
    const expr* rule_l = saved_expr_pool_.make(0);
    const expr* rule_a = saved_expr_pool_.make(1);
    const expr* rule_t = saved_expr_pool_.make(2);
    const expr* zero = saved_expr_pool_.make("zero", {});
    const expr* nil = saved_expr_pool_.make("nil", {});
    const expr* head0 = saved_expr_pool_.make("make_list", {zero, rule_ignored, nil});
    const expr* suc_l = saved_expr_pool_.make("suc", {rule_l});
    const expr* cons_at = saved_expr_pool_.make("cons", {rule_a, rule_t});
    const expr* head1 = saved_expr_pool_.make("make_list", {suc_l, rule_a, cons_at});
    const expr* body1 = saved_expr_pool_.make("make_list", {rule_l, rule_a, rule_t});
    database.push(rule{head0, {}});
    database.push(rule{head1, {body1}});

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
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* _123 = saved_expr_pool_.make("123", {});
    const expr* head = saved_expr_pool_.make("f", {abc, _123});
    database.push(rule{head, {}});

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

TEST_F(BasicManifestIntegrationTest, SimMhuDeactivationRemovesSiblingFromFrontier) {
    /*
     * Intent: choosing one of two incompatible ground heads removes the other from the frontier.
     * initial goals: f(A, B).  (added after set_up in test body)
     * rules:
     *   0: f(abc, 123).
     *   1: f(def, 123).
     */
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* def = saved_expr_pool_.make("def", {});
    const expr* _123 = saved_expr_pool_.make("123", {});
    const expr* head0 = saved_expr_pool_.make("f", {abc, _123});
    const expr* head1 = saved_expr_pool_.make("f", {def, _123});
    database.push(rule{head0, {}});
    database.push(rule{head1, {}});

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
    const rule_id sibling_id = (chosen->idx == 0) ? rule_id{1} : rule_id{0};
    const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
        EXPECT_NE(rl->idx, sibling_id);
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
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* f_head0 = saved_expr_pool_.make("f", {});
    const expr* f_head1 = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});

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
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* _123 = saved_expr_pool_.make("123", {});
    const expr* head = saved_expr_pool_.make("f", {abc, _123});
    database.push(rule{head, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const uint32_t idx_b = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    const expr* var_b = manifest.expr_pool_.make(idx_b);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a, var_b}));


    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    ASSERT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_EQ(*saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_a))),
        *abc);
    EXPECT_EQ(*saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_b))),
        *_123);

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
    const expr* goal_f = saved_expr_pool_.make("f", {});
    const expr* goal_g = saved_expr_pool_.make("g", {});
    const expr* f_head0 = saved_expr_pool_.make("f", {});
    const expr* f_head1 = saved_expr_pool_.make("f", {});
    const expr* g_head2 = saved_expr_pool_.make("g", {});
    const expr* g_head3 = saved_expr_pool_.make("g", {});
    initial_goals.push(goal_f);
    initial_goals.push(goal_g);
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});
    database.push(rule{g_head2, {}});
    database.push(rule{g_head3, {}});

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
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* f_head0 = saved_expr_pool_.make("f", {});
    const expr* f_head1 = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});

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

TEST_F(BasicManifestIntegrationTest, BackloggedCandidateAlreadyDeactivatedOnReelimination) {
    /*
     * Intent: g/1 backlogged before sim (never linked); CDCL elimination of g/1 after f/0 resolves
     * must return already_deactivated, not throw on unlink.
     * initial goals: f.  g.
     * rules:
     *   0: f.   1: g.   2: g.
     * setup: backlog (f, rule_id{1}); learn {f/0, g/1} (f unit; g keeps rule 2).
     */
    const expr* goal_f = saved_expr_pool_.make("f", {});
    const expr* goal_g = saved_expr_pool_.make("g", {});
    const expr* f_head0 = saved_expr_pool_.make("f", {});
    const expr* g_head1 = saved_expr_pool_.make("g", {});
    const expr* g_head2 = saved_expr_pool_.make("g", {});
    initial_goals.push(goal_f);
    initial_goals.push(goal_g);
    database.push(rule{f_head0, {}});
    database.push(rule{g_head1, {}});
    database.push(rule{g_head2, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    const goal_lineage* gl_f = manifest.make_initial_goal_lineage_.make(0);
    const goal_lineage* gl_g = manifest.make_initial_goal_lineage_.make(1);
    const resolution_lineage* rl_f_0 =
        manifest.lineage_pool_.make_resolution_lineage(gl_f, rule_id{0});
    const resolution_lineage* rl_f_1 =
        manifest.lineage_pool_.make_resolution_lineage(gl_f, rule_id{1});
    const resolution_lineage* rl_g_1 =
        manifest.lineage_pool_.make_resolution_lineage(gl_g, rule_id{1});
    manifest.elimination_backlog_.insert_backlogged_elimination(rl_f_1);
    manifest.cdcl_.learn(lemma{{rl_f_0, rl_g_1}});

    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    ASSERT_EQ(sm.consume_yield(), sim_termination::solved);
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
    const expr* goal_f = saved_expr_pool_.make("f", {});
    const expr* goal_g = saved_expr_pool_.make("g", {});
    const expr* f_head0 = saved_expr_pool_.make("f", {});
    const expr* f_head1 = saved_expr_pool_.make("f", {});
    const expr* g_head2 = saved_expr_pool_.make("g", {});
    const expr* g_head3 = saved_expr_pool_.make("g", {});
    initial_goals.push(goal_f);
    initial_goals.push(goal_g);
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});
    database.push(rule{g_head2, {}});
    database.push(rule{g_head3, {}});

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
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* f_head0 = saved_expr_pool_.make("f", {});
    const expr* f_head1 = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});

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
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* head = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
    database.push(rule{head, {}});

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
    const expr* goal = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);

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
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* f_head0 = saved_expr_pool_.make("f", {});
    const expr* f_head1 = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};

    static const BranchGroups kGroups{{rule_id{0}, rule_id{1}}};
    next_branch_until_refuted(
        manifest.solver_,
        manifest.resolution_memory_,
        kGroups,
        {{rule_id{0}}, {rule_id{1}}});
}

TEST_F(BasicManifestIntegrationTest, SolverRefutesAfterEnumeratingAllGroundBranches) {
    /*
     * Intent: after both f branches are found, solver completes with two solved ticks.
     * initial goals: f.
     * rules:
     *   0: f.   1: f.
     */
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* f_head0 = saved_expr_pool_.make("f", {});
    const expr* f_head1 = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};

    static const BranchGroups kGroups{{rule_id{0}, rule_id{1}}};
    next_branch_until_refuted(
        manifest.solver_,
        manifest.resolution_memory_,
        kGroups,
        {{rule_id{0}}, {rule_id{1}}});
}

TEST_F(BasicManifestIntegrationTest, SolverFindsClauseDerivedUnitSolution) {
    /*
     * Intent: f :- g(X). with one g/1 fact solves without a decision.
     * initial goals: f.
     * rules:
     *   0: f :- g(X).
     *   1: g(c).
     */
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* rule_var = saved_expr_pool_.make(0);
    const expr* g_ground = saved_expr_pool_.make("c", {});
    const expr* g_fact = saved_expr_pool_.make("g", {g_ground});
    const expr* f_head = saved_expr_pool_.make("f", {});
    const expr* g_body = saved_expr_pool_.make("g", {rule_var});
    initial_goals.push(goal);
    database.push(rule{f_head, {g_body}});
    database.push(rule{g_fact, {}});

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
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* rule_var = saved_expr_pool_.make(0);
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* xyz = saved_expr_pool_.make("xyz", {});
    const expr* g_fact0 = saved_expr_pool_.make("g", {abc});
    const expr* g_fact1 = saved_expr_pool_.make("g", {xyz});
    const expr* f_head = saved_expr_pool_.make("f", {});
    const expr* g_body = saved_expr_pool_.make("g", {rule_var});
    initial_goals.push(goal);
    database.push(rule{f_head, {g_body}});
    database.push(rule{g_fact0, {}});
    database.push(rule{g_fact1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};

    static const BranchGroups kGroups{{rule_id{1}, rule_id{2}}};
    next_branch_until_refuted(
        manifest.solver_,
        manifest.resolution_memory_,
        kGroups,
        {{rule_id{1}}, {rule_id{2}}});
}

TEST_F(BasicManifestIntegrationTest, SolverFindsSolutionWithCorrectBindings) {
    /*
     * Intent: single-tick solver records correct WHNF bindings for both goal vars.
     * initial goals: f(A, B).
     * rules:
     *   0: f(abc, 123).
     */
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* _123 = saved_expr_pool_.make("123", {});
    const expr* head = saved_expr_pool_.make("f", {abc, _123});
    database.push(rule{head, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const uint32_t idx_b = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    const expr* var_b = manifest.expr_pool_.make(idx_b);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a, var_b}));

    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_TRUE(manifest.decision_memory_.derive().get_resolutions().empty());
    EXPECT_EQ(*saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_a))),
        *abc);
    EXPECT_EQ(*saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_b))),
        *_123);
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
    const expr* rule_var_a = saved_expr_pool_.make(0);
    const expr* rule_var_b = saved_expr_pool_.make(1);
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* _123 = saved_expr_pool_.make("123", {});
    const expr* f_head = saved_expr_pool_.make("f", {rule_var_a, rule_var_b});
    const expr* g_body = saved_expr_pool_.make("g", {rule_var_a});
    const expr* h_body = saved_expr_pool_.make("h", {rule_var_b});
    const expr* g_head = saved_expr_pool_.make("g", {abc});
    const expr* h_head = saved_expr_pool_.make("h", {_123});
    database.push(rule{f_head, {g_body, h_body}});
    database.push(rule{g_head, {}});
    database.push(rule{h_head, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_a = manifest.var_sequencer_.next();
    const uint32_t idx_b = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);
    const expr* var_b = manifest.expr_pool_.make(idx_b);
    initial_goals.push(manifest.expr_pool_.make("f", {var_a, var_b}));

    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_EQ(*saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_a))),
        *abc);
    EXPECT_EQ(*saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_b))),
        *_123);
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
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* xyz = saved_expr_pool_.make("xyz", {});
    const expr* head0 = saved_expr_pool_.make("f", {abc});
    const expr* head1 = saved_expr_pool_.make("f", {xyz});
    database.push(rule{head0, {}});
    database.push(rule{head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};

    const uint32_t idx_a = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);

    initial_goals.push(manifest.expr_pool_.make("f", {var_a}));

    enumerate_all_solutions(
        manifest.solver_,
        expr_printer_.printer,
        manifest.decision_memory_,
        manifest.resolution_memory_,
        {{abc}, {xyz}},
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
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* xyz = saved_expr_pool_.make("xyz", {});
    const expr* head0 = saved_expr_pool_.make("f", {abc});
    const expr* head1 = saved_expr_pool_.make("f", {xyz});
    database.push(rule{head0, {}});
    database.push(rule{head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};

    const uint32_t idx_a = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);

    initial_goals.push(manifest.expr_pool_.make("f", {var_a}));

    next_until_refuted(
        manifest.solver_,
        expr_printer_.printer,
        manifest.decision_memory_,
        manifest.resolution_memory_,
        {{abc}, {xyz}},
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
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* xyz = saved_expr_pool_.make("xyz", {});
    const expr* f_head0 = saved_expr_pool_.make("f", {abc});
    const expr* f_head1 = saved_expr_pool_.make("f", {xyz});
    const expr* g_head0 = saved_expr_pool_.make("g", {abc});
    const expr* g_head1 = saved_expr_pool_.make("g", {xyz});
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});
    database.push(rule{g_head0, {}});
    database.push(rule{g_head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};

    const uint32_t idx_a = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);

    initial_goals.push(manifest.expr_pool_.make("f", {var_a}));
    initial_goals.push(manifest.expr_pool_.make("g", {var_a}));

    enumerate_all_solutions(
        manifest.solver_,
        expr_printer_.printer,
        manifest.decision_memory_,
        manifest.resolution_memory_,
        {{abc}, {xyz}},
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
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* f_head0 = saved_expr_pool_.make("f", {});
    const expr* f_head1 = saved_expr_pool_.make("f", {});
    const expr* f_head2 = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});
    database.push(rule{f_head2, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};

    static const BranchGroups kGroups{{rule_id{0}, rule_id{1}, rule_id{2}}};
    next_branch_until_refuted(
        manifest.solver_,
        manifest.resolution_memory_,
        kGroups,
        {{rule_id{0}}, {rule_id{1}}, {rule_id{2}}});
}

TEST_F(BasicManifestIntegrationTest, SimLifecycleTwoSequentialDecisionsOnTwoGoals) {
    /*
     * Intent: sim records two decisions (one f-branch, one g-branch) on a two-goal problem.
     * initial goals: f.  g.
     * rules:
     *   0: f.   1: f.
     *   2: g.   3: g.
     */
    const expr* goal_f = saved_expr_pool_.make("f", {});
    const expr* goal_g = saved_expr_pool_.make("g", {});
    const expr* f_head0 = saved_expr_pool_.make("f", {});
    const expr* f_head1 = saved_expr_pool_.make("f", {});
    const expr* g_head2 = saved_expr_pool_.make("g", {});
    const expr* g_head3 = saved_expr_pool_.make("g", {});
    initial_goals.push(goal_f);
    initial_goals.push(goal_g);
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});
    database.push(rule{g_head2, {}});
    database.push(rule{g_head3, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);
    EXPECT_EQ(manifest.decision_memory_.count(), 2u);

    const lemma resolution_lemma = manifest.resolution_memory_.derive_resolution_lemma();
    ASSERT_EQ(resolution_lemma.get_resolutions().size(), 2u);

    static const std::set<rule_id> kFBranch{rule_id{0}, rule_id{1}};
    static const std::set<rule_id> kGBranch{rule_id{2}, rule_id{3}};

    std::set<rule_id> f_branches;
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
        if (kFBranch.count(rl->idx))
            f_branches.insert(rl->idx);

    std::set<rule_id> g_branches;
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
        if (kGBranch.count(rl->idx))
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
    const expr* goal_f = saved_expr_pool_.make("f", {});
    const expr* f_head = saved_expr_pool_.make("f", {});
    const expr* g_body = saved_expr_pool_.make("g", {});
    const expr* h_body = saved_expr_pool_.make("h", {});
    const expr* g_head = saved_expr_pool_.make("g", {});
    const expr* h_head = saved_expr_pool_.make("h", {});
    const expr* i_body = saved_expr_pool_.make("i", {});
    const expr* j_body = saved_expr_pool_.make("j", {});
    const expr* i_head = saved_expr_pool_.make("i", {});
    const expr* j_head = saved_expr_pool_.make("j", {});
    initial_goals.push(goal_f);
    database.push(rule{f_head, {g_body, h_body}});
    database.push(rule{g_head, {i_body, j_body}});
    database.push(rule{h_head, {i_body, j_body}});
    database.push(rule{i_head, {}});
    database.push(rule{j_head, {}});

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
    const expr* goal_f = saved_expr_pool_.make("f", {});
    const expr* goal_g = saved_expr_pool_.make("g", {});
    const expr* f_head0 = saved_expr_pool_.make("f", {});
    const expr* f_head1 = saved_expr_pool_.make("f", {});
    const expr* g_head0 = saved_expr_pool_.make("g", {});
    const expr* g_head1 = saved_expr_pool_.make("g", {});
    initial_goals.push(goal_f);
    initial_goals.push(goal_g);
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});
    database.push(rule{g_head0, {}});
    database.push(rule{g_head1, {}});

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

    static const std::set<rule_id> kFBranch{rule_id{0}, rule_id{1}};
    static const std::set<rule_id> kGBranch{rule_id{2}, rule_id{3}};

    std::set<rule_id> f_branches;
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
        if (kFBranch.count(rl->idx))
            f_branches.insert(rl->idx);

    std::set<rule_id> g_branches;
    for (const resolution_lineage* rl : resolution_lemma.get_resolutions())
        if (kGBranch.count(rl->idx))
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
    const expr* goal_f = saved_expr_pool_.make("f", {});
    const expr* goal_g = saved_expr_pool_.make("g", {});
    const expr* f_head0 = saved_expr_pool_.make("f", {});
    const expr* f_head1 = saved_expr_pool_.make("f", {});
    const expr* g_head2 = saved_expr_pool_.make("g", {});
    const expr* g_head3 = saved_expr_pool_.make("g", {});
    initial_goals.push(goal_f);
    initial_goals.push(goal_g);
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});
    database.push(rule{g_head2, {}});
    database.push(rule{g_head3, {}});

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
    const expr* goal_f = saved_expr_pool_.make("f", {});
    const expr* goal_g = saved_expr_pool_.make("g", {});
    const expr* f_head0 = saved_expr_pool_.make("f", {});
    const expr* f_head1 = saved_expr_pool_.make("f", {});
    const expr* g_head2 = saved_expr_pool_.make("g", {});
    const expr* g_head3 = saved_expr_pool_.make("g", {});
    initial_goals.push(goal_f);
    initial_goals.push(goal_g);
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});
    database.push(rule{g_head2, {}});
    database.push(rule{g_head3, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};

    static const BranchGroups kGroups{{rule_id{0}, rule_id{1}}, {rule_id{2}, rule_id{3}}};
    next_branch_until_refuted(
        manifest.solver_,
        manifest.resolution_memory_,
        kGroups,
        {
            {rule_id{0}, rule_id{2}},
            {rule_id{0}, rule_id{3}},
            {rule_id{1}, rule_id{2}},
            {rule_id{1}, rule_id{3}},
        });
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
        const expr* goal_f = saved_expr_pool_.make("f", {});
        const expr* goal_g = saved_expr_pool_.make("g", {});
        const expr* goal_h = saved_expr_pool_.make("h", {});
        const expr* f0 = saved_expr_pool_.make("f", {});
        const expr* f1 = saved_expr_pool_.make("f", {});
        const expr* g0 = saved_expr_pool_.make("g", {});
        const expr* g1 = saved_expr_pool_.make("g", {});
        const expr* h0 = saved_expr_pool_.make("h", {});
        const expr* h1 = saved_expr_pool_.make("h", {});
        seed_goals.push(goal_f);
        seed_goals.push(goal_g);
        seed_goals.push(goal_h);
        seed_db.push(rule{f0, {}});
        seed_db.push(rule{f1, {}});
        seed_db.push(rule{g0, {}});
        seed_db.push(rule{g1, {}});
        seed_db.push(rule{h0, {}});
        seed_db.push(rule{h1, {}});

        basic_manifest manifest{seed_db, seed_goals, kMaxResolutions, seed};

        static const BranchGroups kGroups{
            {rule_id{0}, rule_id{1}}, {rule_id{2}, rule_id{3}}, {rule_id{4}, rule_id{5}}};
        next_branch_until_refuted(
            manifest.solver_,
            manifest.resolution_memory_,
            kGroups,
            {
                {rule_id{0}, rule_id{2}, rule_id{4}},
                {rule_id{0}, rule_id{2}, rule_id{5}},
                {rule_id{0}, rule_id{3}, rule_id{4}},
                {rule_id{0}, rule_id{3}, rule_id{5}},
                {rule_id{1}, rule_id{2}, rule_id{4}},
                {rule_id{1}, rule_id{2}, rule_id{5}},
                {rule_id{1}, rule_id{3}, rule_id{4}},
                {rule_id{1}, rule_id{3}, rule_id{5}},
            });
    }
}

TEST_F(BasicManifestIntegrationTest, SolverEnumeratesFourVarBindingSolutions) {
    /*
     * Intent: solver enumerates four distinct bindings for f(A) across four ground heads.
     * initial goals: f(A).
     * rules:
     *   0: f(abc).   1: f(xyz).   2: f(def).   3: f(ghi).
     */
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* xyz = saved_expr_pool_.make("xyz", {});
    const expr* def = saved_expr_pool_.make("def", {});
    const expr* ghi = saved_expr_pool_.make("ghi", {});
    const expr* head0 = saved_expr_pool_.make("f", {abc});
    const expr* head1 = saved_expr_pool_.make("f", {xyz});
    const expr* head2 = saved_expr_pool_.make("f", {def});
    const expr* head3 = saved_expr_pool_.make("f", {ghi});
    database.push(rule{head0, {}});
    database.push(rule{head1, {}});
    database.push(rule{head2, {}});
    database.push(rule{head3, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};

    const uint32_t idx_a = manifest.var_sequencer_.next();
    const expr* var_a = manifest.expr_pool_.make(idx_a);

    initial_goals.push(manifest.expr_pool_.make("f", {var_a}));

    next_until_refuted(
        manifest.solver_,
        expr_printer_.printer,
        manifest.decision_memory_,
        manifest.resolution_memory_,
        {{abc}, {xyz}, {def}, {ghi}},
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
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* rule_var = saved_expr_pool_.make(0);
    const expr* a = saved_expr_pool_.make("a", {});
    const expr* b = saved_expr_pool_.make("b", {});
    const expr* c = saved_expr_pool_.make("c", {});
    const expr* d = saved_expr_pool_.make("d", {});
    const expr* g_fact1 = saved_expr_pool_.make("g", {a});
    const expr* g_fact2 = saved_expr_pool_.make("g", {b});
    const expr* g_fact3 = saved_expr_pool_.make("g", {c});
    const expr* g_fact4 = saved_expr_pool_.make("g", {d});
    const expr* f_head = saved_expr_pool_.make("f", {});
    const expr* g_body = saved_expr_pool_.make("g", {rule_var});
    initial_goals.push(goal);
    database.push(rule{f_head, {g_body}});
    database.push(rule{g_fact1, {}});
    database.push(rule{g_fact2, {}});
    database.push(rule{g_fact3, {}});
    database.push(rule{g_fact4, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};

    static const BranchGroups kGroups{{rule_id{1}, rule_id{2}, rule_id{3}, rule_id{4}}};
    next_branch_until_refuted(
        manifest.solver_,
        manifest.resolution_memory_,
        kGroups,
        {{rule_id{1}}, {rule_id{2}}, {rule_id{3}}, {rule_id{4}}});
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
        const expr* abc = saved_expr_pool_.make("abc", {});
        const expr* def = saved_expr_pool_.make("def", {});
        const expr* ghi = saved_expr_pool_.make("ghi", {});
        const expr* jkl = saved_expr_pool_.make("jkl", {});
        const expr* mno = saved_expr_pool_.make("mno", {});
        const expr* pqr = saved_expr_pool_.make("pqr", {});
        const expr* xyz = saved_expr_pool_.make("xyz", {});
        const expr* f_head0 = saved_expr_pool_.make("f", {});
        const expr* f_head1 = saved_expr_pool_.make("f", {});
        const expr* g_abc = saved_expr_pool_.make("g", {abc, xyz, pqr});
        const expr* g_def = saved_expr_pool_.make("g", {def, xyz, pqr});
        const expr* g_ghi = saved_expr_pool_.make("g", {ghi, xyz, pqr});
        const expr* g_jkl = saved_expr_pool_.make("g", {jkl, xyz, pqr});
        const expr* g_mno = saved_expr_pool_.make("g", {mno, xyz, pqr});
        seed_db.push(rule{f_head0, {}});
        seed_db.push(rule{f_head1, {}});
        seed_db.push(rule{g_abc, {}});
        seed_db.push(rule{g_def, {}});
        seed_db.push(rule{g_ghi, {}});
        seed_db.push(rule{g_jkl, {}});
        seed_db.push(rule{g_mno, {}});

        basic_manifest manifest{seed_db, seed_goals, kMaxResolutions, seed};
        const uint32_t idx_a = manifest.var_sequencer_.next();
        const uint32_t idx_b = manifest.var_sequencer_.next();
        const uint32_t idx_c = manifest.var_sequencer_.next();
        const expr* var_a = manifest.expr_pool_.make(idx_a);
        const expr* var_b = manifest.expr_pool_.make(idx_b);
        const expr* var_c = manifest.expr_pool_.make(idx_c);
        seed_goals.push(manifest.expr_pool_.make("f", {}));
        seed_goals.push(manifest.expr_pool_.make("g", {var_a, var_b, var_c}));

        static const BranchGroups kBranchGroups{
            {rule_id{0}, rule_id{1}}, {rule_id{2}, rule_id{3}, rule_id{4}, rule_id{5}, rule_id{6}}};
        next_branch_until_refuted(
            manifest.solver_,
            manifest.resolution_memory_,
            kBranchGroups,
            {
                {rule_id{0}, rule_id{2}}, {rule_id{0}, rule_id{3}}, {rule_id{0}, rule_id{4}},
                {rule_id{0}, rule_id{5}}, {rule_id{0}, rule_id{6}}, {rule_id{1}, rule_id{2}},
                {rule_id{1}, rule_id{3}}, {rule_id{1}, rule_id{4}}, {rule_id{1}, rule_id{5}},
                {rule_id{1}, rule_id{6}},
            });

        db binding_db;
        initial_goal_exprs binding_goals;
        binding_db.push(rule{f_head0, {}});
        binding_db.push(rule{f_head1, {}});
        binding_db.push(rule{g_abc, {}});
        binding_db.push(rule{g_def, {}});
        binding_db.push(rule{g_ghi, {}});
        binding_db.push(rule{g_jkl, {}});
        binding_db.push(rule{g_mno, {}});

        basic_manifest binding_manifest{binding_db, binding_goals, kMaxResolutions, seed};
        normalizer binding_normalizer{binding_manifest.loc_};
        const uint32_t idx_a_bind = binding_manifest.var_sequencer_.next();
        const uint32_t idx_b_bind = binding_manifest.var_sequencer_.next();
        const uint32_t idx_c_bind = binding_manifest.var_sequencer_.next();
        const expr* var_a_bind = binding_manifest.expr_pool_.make(idx_a_bind);
        const expr* var_b_bind = binding_manifest.expr_pool_.make(idx_b_bind);
        const expr* var_c_bind = binding_manifest.expr_pool_.make(idx_c_bind);
        binding_goals.push(binding_manifest.expr_pool_.make("f", {}));
        binding_goals.push(binding_manifest.expr_pool_.make("g", {var_a_bind, var_b_bind, var_c_bind}));

        std::vector<solution> binding_solutions;
        auto binding_sm = binding_manifest.solver_.solve();
        while (true) {
            binding_sm.resume();
            if (!binding_sm.has_yield())
                break;
            if (binding_sm.consume_yield() != sim_termination::solved)
                continue;
            binding_solutions.push_back({
                saved_expr_pool_.import(binding_normalizer.normalize(var_a_bind)),
                saved_expr_pool_.import(binding_normalizer.normalize(var_b_bind)),
                saved_expr_pool_.import(binding_normalizer.normalize(var_c_bind)),
            });
        }
        ASSERT_EQ(binding_solutions.size(), kRawSolutions);
        size_t saw_abc = 0;
        size_t saw_def = 0;
        size_t saw_ghi = 0;
        size_t saw_jkl = 0;
        size_t saw_mno = 0;
        for (const solution& s : binding_solutions) {
            ASSERT_EQ(s.size(), 3u);
            const expr& a_bound = *s[0];
            if (a_bound == *abc)
                ++saw_abc;
            else if (a_bound == *def)
                ++saw_def;
            else if (a_bound == *ghi)
                ++saw_ghi;
            else if (a_bound == *jkl)
                ++saw_jkl;
            else if (a_bound == *mno)
                ++saw_mno;
            else
                FAIL() << "unexpected binding for idx_a";
            EXPECT_EQ(*s[1], *xyz);
            EXPECT_EQ(*s[2], *pqr);
        }
        EXPECT_EQ(saw_abc, 2u);
        EXPECT_EQ(saw_def, 2u);
        EXPECT_EQ(saw_ghi, 2u);
        EXPECT_EQ(saw_jkl, 2u);
        EXPECT_EQ(saw_mno, 2u);
    }
}

// Tier S — CHC enumeration scenarios (legacy integration tests 5–19)

TEST_F(BasicManifestIntegrationTest, RefutesAfterCdclOnUnsatClauseBranches) {
    /*
     * Intent: multi-tick runtime CDCL refutation on unsatisfiable a :- b / a :- c (NOT empty-DB refutation).
     * initial goals: a.
     * rules:
     *   0: a :- b.   1: a :- c.
     */
    const expr* b = saved_expr_pool_.make("b", {});
    const expr* c = saved_expr_pool_.make("c", {});
    const expr* a_head0 = saved_expr_pool_.make("a", {});
    const expr* a_head1 = saved_expr_pool_.make("a", {});
    database.push(rule{a_head0, {b}});
    database.push(rule{a_head1, {c}});

    const expr* goal = saved_expr_pool_.make("a", {});
    initial_goals.push(goal);

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

TEST_F(BasicManifestIntegrationTest, FindsUniqueSharedVarConjunctionThenRefutes) {
    /*
     * Intent: is_a(X) ∧ is_b(X) has unique binding X=2, then refutes.
     * initial goals: is_a(X).  is_b(X).
     * rules:
     *   0: is_a(1).   1: is_a(2).   2: is_b(2).   3: is_b(3).
     */
    const expr* one = saved_expr_pool_.make("1", {});
    const expr* two = saved_expr_pool_.make("2", {});
    const expr* three = saved_expr_pool_.make("3", {});
    const expr* is_a1 = saved_expr_pool_.make("is_a", {one});
    const expr* is_a2 = saved_expr_pool_.make("is_a", {two});
    const expr* is_b2 = saved_expr_pool_.make("is_b", {two});
    const expr* is_b3 = saved_expr_pool_.make("is_b", {three});
    database.push(rule{is_a1, {}});
    database.push(rule{is_a2, {}});
    database.push(rule{is_b2, {}});
    database.push(rule{is_b3, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_x = manifest.var_sequencer_.next();
    const expr* var_x = manifest.expr_pool_.make(idx_x);
    initial_goals.push(manifest.expr_pool_.make("is_a", {var_x}));
    initial_goals.push(manifest.expr_pool_.make("is_b", {var_x}));

    next_until_refuted(
        manifest.solver_,
        expr_printer_.printer,
        manifest.decision_memory_,
        manifest.resolution_memory_,
        {{two}},
        [&]() -> solution {
            return {saved_expr_pool_.import(
                normalizer.normalize(manifest.expr_pool_.make(idx_x)))};
        });
}

TEST_F(BasicManifestIntegrationTest, EnumeratesTwoParentBindingsForAlice) {
    /*
     * Intent: parent(X, alice) enumerates bob and carol; parent(dave,bob) is head-elim junk.
     * initial goals: parent(X, alice).
     * rules:
     *   0: parent(bob, alice).   1: parent(carol, alice).   2: parent(dave, bob).
     */
    const expr* bob = saved_expr_pool_.make("bob", {});
    const expr* carol = saved_expr_pool_.make("carol", {});
    const expr* alice = saved_expr_pool_.make("alice", {});
    const expr* dave = saved_expr_pool_.make("dave", {});
    const expr* parent_bob_alice = saved_expr_pool_.make("parent", {bob, alice});
    const expr* parent_carol_alice = saved_expr_pool_.make("parent", {carol, alice});
    const expr* parent_dave_bob = saved_expr_pool_.make("parent", {dave, bob});
    database.push(rule{parent_bob_alice, {}});
    database.push(rule{parent_carol_alice, {}});
    database.push(rule{parent_dave_bob, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    normalizer normalizer{manifest.loc_};
    const uint32_t idx_x = manifest.var_sequencer_.next();
    const expr* var_x = manifest.expr_pool_.make(idx_x);
    const expr* alice_goal = manifest.expr_pool_.make("alice", {});
    initial_goals.push(manifest.expr_pool_.make("parent", {var_x, alice_goal}));

    next_until_refuted(
        manifest.solver_,
        expr_printer_.printer,
        manifest.decision_memory_,
        manifest.resolution_memory_,
        {{bob}, {carol}},
        [&]() -> solution {
            return {saved_expr_pool_.import(
                normalizer.normalize(manifest.expr_pool_.make(idx_x)))};
        });
}

TEST_F(BasicManifestIntegrationTest, EnumeratesPeanoLessThanSeven) {
    /*
     * Intent: lt(N, suc^7(zero)) — seven Peano solutions N ∈ {0..6}.
     * Ported from legacy integration test 14.
     * initial goals: lt(N, seven).
     * rules:
     *   0: nat(zero).   1: nat(suc(X)) :- nat(X).
     *   2: lt(zero, suc(X)) :- nat(X).   3: lt(suc(X), suc(Y)) :- lt(X, Y).
     */
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    auto push_peano_lt_rules = [&]() {
        const expr* zero = saved_expr_pool_.make("zero", {});
        database.push(rule{saved_expr_pool_.make("nat", {zero}), {}});

        const expr* rv1 = saved_expr_pool_.make(0);
        const expr* suc_rv1 = saved_expr_pool_.make("suc", {rv1});
        database.push(rule{saved_expr_pool_.make("nat", {suc_rv1}), {saved_expr_pool_.make("nat", {rv1})}});

        const expr* rv2 = saved_expr_pool_.make(0);
        const expr* suc_rv2 = saved_expr_pool_.make("suc", {rv2});
        database.push(rule{
            saved_expr_pool_.make("lt", {zero, suc_rv2}), {saved_expr_pool_.make("nat", {rv2})}});

        const expr* rv3 = saved_expr_pool_.make(0);
        const expr* rv4 = saved_expr_pool_.make(1);
        const expr* suc_rv3 = saved_expr_pool_.make("suc", {rv3});
        const expr* suc_rv4 = saved_expr_pool_.make("suc", {rv4});
        database.push(rule{
            saved_expr_pool_.make("lt", {suc_rv3, suc_rv4}),
            {saved_expr_pool_.make("lt", {rv3, rv4})}});
    };
    push_peano_lt_rules();

    std::set<solution> expected;
    for (int n = 0; n < 7; ++n)
        expected.insert({peano_saved(n)});

    basic_manifest manifest{database, initial_goals, kPeanoBudget, kSeed};
    normalizer normalizer{manifest.loc_};

    const uint32_t idx_n = manifest.var_sequencer_.next();
    const expr* var_n = manifest.expr_pool_.make(idx_n);

    const expr* seven = manifest.expr_pool_.make("zero", {});
    for (int i = 0; i < 7; ++i)
        seven = manifest.expr_pool_.make("suc", {seven});
    initial_goals.push(manifest.expr_pool_.make("lt", {var_n, seven}));

    next_until_refuted(
        manifest.solver_,
        expr_printer_.printer,
        manifest.decision_memory_,
        manifest.resolution_memory_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(
                normalizer.normalize(manifest.expr_pool_.make(idx_n)))};
        });
}

TEST_F(BasicManifestIntegrationTest, EnumeratesSatPAndQOrR) {
    /*
     * Intent: P ∧ (Q ∨ R) — three bool assignments (calibration probe for multi-var relational CHC).
     * Ported from legacy integration test 11.
     * initial goals: bool(P). bool(Q). bool(R). or(Q,R,QR). and(P,QR,true).
     * rules: bool(true/false); relational or/and with bool(X) bodies.
     */
    const expr* true_atom = saved_expr_pool_.make("true", {});
    const expr* false_atom = saved_expr_pool_.make("false", {});
    const expr* bool_true = saved_expr_pool_.make("bool", {true_atom});
    const expr* bool_false = saved_expr_pool_.make("bool", {false_atom});
    database.push(rule{bool_true, {}});
    database.push(rule{bool_false, {}});

    const expr* or_rv = saved_expr_pool_.make(0);
    const expr* or_true_x_true = saved_expr_pool_.make("or", {true_atom, or_rv, true_atom});
    const expr* or_bool_body = saved_expr_pool_.make("bool", {or_rv});
    database.push(rule{or_true_x_true, {or_bool_body}});

    const expr* or_rv2 = saved_expr_pool_.make(0);
    const expr* or_false_x_x = saved_expr_pool_.make("or", {false_atom, or_rv2, or_rv2});
    const expr* or_bool_body2 = saved_expr_pool_.make("bool", {or_rv2});
    database.push(rule{or_false_x_x, {or_bool_body2}});

    const expr* and_rv = saved_expr_pool_.make(0);
    const expr* and_true_x_x = saved_expr_pool_.make("and", {true_atom, and_rv, and_rv});
    const expr* and_bool_body = saved_expr_pool_.make("bool", {and_rv});
    database.push(rule{and_true_x_x, {and_bool_body}});

    const expr* and_rv2 = saved_expr_pool_.make(0);
    const expr* and_false_x_false = saved_expr_pool_.make("and", {false_atom, and_rv2, false_atom});
    const expr* and_bool_body2 = saved_expr_pool_.make("bool", {and_rv2});
    database.push(rule{and_false_x_false, {and_bool_body2}});

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

    // Raw solved ticks may exceed 3 when resolution paths duplicate bindings.
    next_until_refuted(
        manifest.solver_,
        expr_printer_.printer,
        manifest.decision_memory_,
        manifest.resolution_memory_,
        {{true_atom, true_atom, true_atom}, {true_atom, true_atom, false_atom}, {true_atom, false_atom, true_atom}},
        [&]() -> solution {
            return {
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_p))),
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_q))),
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_r))),
            };
        });
}

TEST_F(BasicManifestIntegrationTest, EnumeratesTwoSatAssignmentsForImpliesQ) {
    /*
     * Intent: (P∨Q)∧(¬P∨Q) — both solutions have Q=true; P differs.
     * Ported from legacy integration test 8.
     * initial goals: bool(P). bool(Q). or(P,Q,true). not(P,NP). or(NP,Q,true).
     * rules: 8 bool/or/not ground facts.
     */
    const expr* true_atom = saved_expr_pool_.make("true", {});
    const expr* false_atom = saved_expr_pool_.make("false", {});
    const expr* bool_true = saved_expr_pool_.make("bool", {true_atom});
    const expr* bool_false = saved_expr_pool_.make("bool", {false_atom});
    const expr* not_true_false = saved_expr_pool_.make("not", {true_atom, false_atom});
    const expr* not_false_true = saved_expr_pool_.make("not", {false_atom, true_atom});
    const expr* or_t_t_t = saved_expr_pool_.make("or", {true_atom, true_atom, true_atom});
    const expr* or_t_f_t = saved_expr_pool_.make("or", {true_atom, false_atom, true_atom});
    const expr* or_f_t_t = saved_expr_pool_.make("or", {false_atom, true_atom, true_atom});
    const expr* or_f_f_f = saved_expr_pool_.make("or", {false_atom, false_atom, false_atom});
    database.push(rule{bool_true, {}});
    database.push(rule{bool_false, {}});
    database.push(rule{not_true_false, {}});
    database.push(rule{not_false_true, {}});
    database.push(rule{or_t_t_t, {}});
    database.push(rule{or_t_f_t, {}});
    database.push(rule{or_f_t_t, {}});
    database.push(rule{or_f_f_f, {}});

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

TEST_F(BasicManifestIntegrationTest, EnumeratesTwoPathTwoColorings) {
    /*
     * Intent: 2-color a 3-node path A-B-C — property checks, not set enumeration.
     * Ported from legacy integration test 9.
     * initial goals: color(A). color(B). color(C). diff(A,B). diff(B,C).
     * rules: color(red/blue). diff(red,blue). diff(blue,red).
     */
    const expr* red = saved_expr_pool_.make("red", {});
    const expr* blue = saved_expr_pool_.make("blue", {});
    const expr* color_red = saved_expr_pool_.make("color", {red});
    const expr* color_blue = saved_expr_pool_.make("color", {blue});
    const expr* diff_red_blue = saved_expr_pool_.make("diff", {red, blue});
    const expr* diff_blue_red = saved_expr_pool_.make("diff", {blue, red});
    database.push(rule{color_red, {}});
    database.push(rule{color_blue, {}});
    database.push(rule{diff_red_blue, {}});
    database.push(rule{diff_blue_red, {}});

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

TEST_F(BasicManifestIntegrationTest, EnumeratesK3ThreeColorings) {
    /*
     * Intent: enumerate all 3! proper colorings of K3 over colors {red, green, blue}.
     * Ported from legacy integration test 10.
     * initial goals: color(A). color(B). color(C). diff(A,B). diff(A,C). diff(B,C).
     * rules: color(red/green/blue). diff(x,y) for all ordered x != y pairs.
     */
    static constexpr std::array<uint32_t, 10> kSeeds = {
        0, 1, 7, 13, 42, 99, 123, 256, 1000, 9999};

    for (uint32_t seed : kSeeds) {
        SCOPED_TRACE(testing::Message() << "seed=" << seed);

        db seed_db;
        initial_goal_exprs seed_goals;
        const expr* red = saved_expr_pool_.make("red", {});
        const expr* green = saved_expr_pool_.make("green", {});
        const expr* blue = saved_expr_pool_.make("blue", {});
        const expr* color_red = saved_expr_pool_.make("color", {red});
        const expr* color_green = saved_expr_pool_.make("color", {green});
        const expr* color_blue = saved_expr_pool_.make("color", {blue});
        seed_db.push(rule{color_red, {}});
        seed_db.push(rule{color_green, {}});
        seed_db.push(rule{color_blue, {}});

        const expr* diff_red_green = saved_expr_pool_.make("diff", {red, green});
        const expr* diff_red_blue = saved_expr_pool_.make("diff", {red, blue});
        const expr* diff_green_red = saved_expr_pool_.make("diff", {green, red});
        const expr* diff_green_blue = saved_expr_pool_.make("diff", {green, blue});
        const expr* diff_blue_red = saved_expr_pool_.make("diff", {blue, red});
        const expr* diff_blue_green = saved_expr_pool_.make("diff", {blue, green});
        seed_db.push(rule{diff_red_green, {}});
        seed_db.push(rule{diff_red_blue, {}});
        seed_db.push(rule{diff_green_red, {}});
        seed_db.push(rule{diff_green_blue, {}});
        seed_db.push(rule{diff_blue_red, {}});
        seed_db.push(rule{diff_blue_green, {}});

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

        std::set<solution> expected = {
            {red, green, blue}, {red, blue, green}, {green, red, blue}, {green, blue, red}, {blue, red, green}, {blue, green, red},
        };

        next_until_refuted(
            manifest.solver_,
            expr_printer_.printer,
            manifest.decision_memory_,
            manifest.resolution_memory_,
            expected,
            [&]() -> solution {
                return {
                    saved_expr_pool_.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_a))),
                    saved_expr_pool_.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_b))),
                    saved_expr_pool_.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_c))),
                };
            });
    }
}

TEST_F(BasicManifestIntegrationTest, EnumeratesK3TailFourNodeColorings) {
    /*
     * Intent: 12 colorings — K3 on (A,B,C) plus tail D with diff(A,D).
     * Ported from legacy integration test 12.
     */
    static constexpr std::array<uint32_t, 10> kSeeds = {
        0, 1, 7, 13, 42, 99, 123, 256, 1000, 9999};
    for (uint32_t seed : kSeeds) {
        SCOPED_TRACE(testing::Message() << "seed=" << seed);
        db seed_db;
        initial_goal_exprs seed_goals;
        const expr* red = saved_expr_pool_.make("red", {});
        const expr* green = saved_expr_pool_.make("green", {});
        const expr* blue = saved_expr_pool_.make("blue", {});
        const expr* color_red = saved_expr_pool_.make("color", {red});
        const expr* color_green = saved_expr_pool_.make("color", {green});
        const expr* color_blue = saved_expr_pool_.make("color", {blue});
        seed_db.push(rule{color_red, {}});
        seed_db.push(rule{color_green, {}});
        seed_db.push(rule{color_blue, {}});
        const expr* diff_rg = saved_expr_pool_.make("diff", {red, green});
        const expr* diff_rb = saved_expr_pool_.make("diff", {red, blue});
        const expr* diff_gr = saved_expr_pool_.make("diff", {green, red});
        const expr* diff_gb = saved_expr_pool_.make("diff", {green, blue});
        const expr* diff_br = saved_expr_pool_.make("diff", {blue, red});
        const expr* diff_bg = saved_expr_pool_.make("diff", {blue, green});
        seed_db.push(rule{diff_rg, {}});
        seed_db.push(rule{diff_rb, {}});
        seed_db.push(rule{diff_gr, {}});
        seed_db.push(rule{diff_gb, {}});
        seed_db.push(rule{diff_br, {}});
        seed_db.push(rule{diff_bg, {}});
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
        std::set<solution> expected = {
            {red, green, blue, green}, {red, green, blue, blue}, {red, blue, green, green}, {red, blue, green, blue},
            {green, red, blue, red}, {green, red, blue, blue}, {green, blue, red, red}, {green, blue, red, blue},
            {blue, red, green, red}, {blue, red, green, green}, {blue, green, red, red}, {blue, green, red, green},
        };
        next_until_refuted(manifest.solver_, expr_printer_.printer, manifest.decision_memory_, manifest.resolution_memory_, expected, [&]() -> solution {
            return {
                saved_expr_pool_.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_a))),
                saved_expr_pool_.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_b))),
                saved_expr_pool_.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_c))),
                saved_expr_pool_.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_d))),
            };
        });
    }
}
TEST_F(BasicManifestIntegrationTest, EnumeratesFourVarSatThreeClauses) {
    /*
     * Intent: satisfy (P ∨ Q) ∧ (R ∨ S) ∧ (¬P ∨ ¬R).
     * Ported from legacy integration test 13.
     */
    static constexpr std::array<uint32_t, 10> kSeeds = {
        0, 1, 7, 13, 42, 99, 123, 256, 1000, 9999};
    static constexpr size_t kSatBudget = 256;

    for (uint32_t seed : kSeeds) {
        SCOPED_TRACE(testing::Message() << "seed=" << seed);

        db seed_db;
        initial_goal_exprs seed_goals;
        const expr* true_atom = saved_expr_pool_.make("true", {});
        const expr* false_atom = saved_expr_pool_.make("false", {});
        const expr* bool_true = saved_expr_pool_.make("bool", {true_atom});
        const expr* bool_false = saved_expr_pool_.make("bool", {false_atom});
        const expr* not_true_false = saved_expr_pool_.make("not", {true_atom, false_atom});
        const expr* not_false_true = saved_expr_pool_.make("not", {false_atom, true_atom});
        seed_db.push(rule{bool_true, {}});
        seed_db.push(rule{bool_false, {}});
        seed_db.push(rule{not_true_false, {}});
        seed_db.push(rule{not_false_true, {}});

        const expr* or_t_t_t = saved_expr_pool_.make("or", {true_atom, true_atom, true_atom});
        const expr* or_t_f_t = saved_expr_pool_.make("or", {true_atom, false_atom, true_atom});
        const expr* or_f_t_t = saved_expr_pool_.make("or", {false_atom, true_atom, true_atom});
        const expr* or_f_f_f = saved_expr_pool_.make("or", {false_atom, false_atom, false_atom});
        seed_db.push(rule{or_t_t_t, {}});
        seed_db.push(rule{or_t_f_t, {}});
        seed_db.push(rule{or_f_t_t, {}});
        seed_db.push(rule{or_f_f_f, {}});

        const expr* and_t_t_t = saved_expr_pool_.make("and", {true_atom, true_atom, true_atom});
        const expr* and_t_f_f = saved_expr_pool_.make("and", {true_atom, false_atom, false_atom});
        const expr* and_f_t_f = saved_expr_pool_.make("and", {false_atom, true_atom, false_atom});
        const expr* and_f_f_f = saved_expr_pool_.make("and", {false_atom, false_atom, false_atom});
        seed_db.push(rule{and_t_t_t, {}});
        seed_db.push(rule{and_t_f_f, {}});
        seed_db.push(rule{and_f_t_f, {}});
        seed_db.push(rule{and_f_f_f, {}});

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

        std::set<solution> expected = {
            {true_atom, true_atom, false_atom, true_atom},
            {true_atom, false_atom, false_atom, true_atom},
            {false_atom, true_atom, true_atom, true_atom},
            {false_atom, true_atom, true_atom, false_atom},
            {false_atom, true_atom, false_atom, true_atom},
        };

        next_until_refuted(
            manifest.solver_,
            expr_printer_.printer,
            manifest.decision_memory_,
            manifest.resolution_memory_,
            expected,
            [&]() -> solution {
                return {
                    saved_expr_pool_.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_p))),
                    saved_expr_pool_.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_q))),
                    saved_expr_pool_.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_r))),
                    saved_expr_pool_.import(seed_normalizer.normalize(manifest.expr_pool_.make(idx_s))),
                };
            });
    }
}

TEST_F(BasicManifestIntegrationTest, EnumeratesAddPairsSummingLessThanTen) {
    /*
     * Intent: enumerate all (X,Y) where add(X,Y,S) and lt(S,10) (55 pairs).
     * Ported from legacy integration test 15.
     */
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    auto push_peano_add_lt_rules = [&]() {
        const expr* zero = saved_expr_pool_.make("zero", {});
        const expr* nat_zero = saved_expr_pool_.make("nat", {zero});
        database.push(rule{nat_zero, {}});

        const expr* rv1 = saved_expr_pool_.make(0);
        const expr* suc_rv1 = saved_expr_pool_.make("suc", {rv1});
        const expr* nat_suc = saved_expr_pool_.make("nat", {suc_rv1});
        const expr* nat_rv1 = saved_expr_pool_.make("nat", {rv1});
        database.push(rule{nat_suc, {nat_rv1}});

        const expr* rv2 = saved_expr_pool_.make(0);
        const expr* add_zero_y_y = saved_expr_pool_.make("add", {zero, rv2, rv2});
        const expr* nat_rv2 = saved_expr_pool_.make("nat", {rv2});
        database.push(rule{add_zero_y_y, {nat_rv2}});

        const expr* rv3 = saved_expr_pool_.make(0);
        const expr* rv4 = saved_expr_pool_.make(1);
        const expr* rv5 = saved_expr_pool_.make(2);
        const expr* suc_rv3 = saved_expr_pool_.make("suc", {rv3});
        const expr* suc_rv5 = saved_expr_pool_.make("suc", {rv5});
        const expr* add_suc = saved_expr_pool_.make("add", {suc_rv3, rv4, suc_rv5});
        const expr* add_body = saved_expr_pool_.make("add", {rv3, rv4, rv5});
        database.push(rule{add_suc, {add_body}});

        const expr* rv6 = saved_expr_pool_.make(0);
        const expr* suc_rv6 = saved_expr_pool_.make("suc", {rv6});
        const expr* lt_zero_suc = saved_expr_pool_.make("lt", {zero, suc_rv6});
        const expr* nat_rv6 = saved_expr_pool_.make("nat", {rv6});
        database.push(rule{lt_zero_suc, {nat_rv6}});

        const expr* rv7 = saved_expr_pool_.make(0);
        const expr* rv8 = saved_expr_pool_.make(1);
        const expr* suc_rv7 = saved_expr_pool_.make("suc", {rv7});
        const expr* suc_rv8 = saved_expr_pool_.make("suc", {rv8});
        const expr* lt_suc_suc = saved_expr_pool_.make("lt", {suc_rv7, suc_rv8});
        const expr* lt_body = saved_expr_pool_.make("lt", {rv7, rv8});
        database.push(rule{lt_suc_suc, {lt_body}});
    };
    push_peano_add_lt_rules();

    std::set<solution> expected;
    for (int x = 0; x < 10; ++x) {
        for (int y = 0; y < 10 - x; ++y)
            expected.insert({peano_saved(x), peano_saved(y)});
    }
    ASSERT_EQ(expected.size(), 55u);

    initial_goal_exprs probe_goals;
    basic_manifest probe_manifest{database, probe_goals, kPeanoBudget, kSeed};
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

    next_until_refuted(
        probe_manifest.solver_,
        expr_printer_.printer,
        probe_manifest.decision_memory_,
        probe_manifest.resolution_memory_,
        expected,
        [&]() -> solution {
            return {
                saved_expr_pool_.import(
                    probe_normalizer.normalize(probe_manifest.expr_pool_.make(idx_x_probe))),
                saved_expr_pool_.import(
                    probe_normalizer.normalize(probe_manifest.expr_pool_.make(idx_y_probe))),
            };
        });

    initial_goal_exprs solve_goals;
    basic_manifest manifest{database, solve_goals, kPeanoBudget, kSeed};
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
        expr_printer_.printer,
        manifest.decision_memory_,
        manifest.resolution_memory_,
        expected,
        [&]() -> solution {
            return {
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_x))),
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_y))),
            };
        });
}

TEST_F(BasicManifestIntegrationTest, EnumeratesAddPairsSummingExactlyTen) {
    /*
     * Intent: enumerate all (X,Y) where add(X,Y,10) (11 pairs).
     * Ported from legacy integration test 16.
     */
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    auto push_peano_add_rules = [&]() {
        const expr* zero = saved_expr_pool_.make("zero", {});
        database.push(rule{saved_expr_pool_.make("nat", {zero}), {}});

        const expr* rv1 = saved_expr_pool_.make(0);
        const expr* suc_rv1 = saved_expr_pool_.make("suc", {rv1});
        database.push(rule{
            saved_expr_pool_.make("nat", {suc_rv1}), {saved_expr_pool_.make("nat", {rv1})}});

        const expr* rv2 = saved_expr_pool_.make(0);
        const expr* add_zero_y_y = saved_expr_pool_.make("add", {zero, rv2, rv2});
        database.push(rule{add_zero_y_y, {saved_expr_pool_.make("nat", {rv2})}});

        const expr* rv3 = saved_expr_pool_.make(0);
        const expr* rv4 = saved_expr_pool_.make(1);
        const expr* rv5 = saved_expr_pool_.make(2);
        const expr* suc_rv3 = saved_expr_pool_.make("suc", {rv3});
        const expr* suc_rv5 = saved_expr_pool_.make("suc", {rv5});
        database.push(rule{
            saved_expr_pool_.make("add", {suc_rv3, rv4, suc_rv5}),
            {saved_expr_pool_.make("add", {rv3, rv4, rv5})}});
    };
    push_peano_add_rules();

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
        expr_printer_.printer,
        manifest.decision_memory_,
        manifest.resolution_memory_,
        expected,
        [&]() -> solution {
            return {
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_x))),
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_y))),
            };
        });
}

TEST_F(BasicManifestIntegrationTest, EnumeratesMulPairsProductEight) {
    /*
     * Intent: enumerate all factor pairs (X,Y) where mul(X,Y,8) (4 pairs).
     * Ported from legacy integration test 17.
     */
    static constexpr size_t kPeanoBudget = 256;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make("zero", {});
    const expr* nat_zero = saved_expr_pool_.make("nat", {zero});
    database.push(rule{nat_zero, {}});

    const expr* rv1 = saved_expr_pool_.make(0);
    const expr* suc_rv1 = saved_expr_pool_.make("suc", {rv1});
    const expr* nat_suc = saved_expr_pool_.make("nat", {suc_rv1});
    const expr* nat_rv1 = saved_expr_pool_.make("nat", {rv1});
    database.push(rule{nat_suc, {nat_rv1}});

    const expr* rv2 = saved_expr_pool_.make(0);
    const expr* add_zero_y_y = saved_expr_pool_.make("add", {zero, rv2, rv2});
    const expr* nat_rv2 = saved_expr_pool_.make("nat", {rv2});
    database.push(rule{add_zero_y_y, {nat_rv2}});

    const expr* rv3 = saved_expr_pool_.make(0);
    const expr* rv4 = saved_expr_pool_.make(1);
    const expr* rv5 = saved_expr_pool_.make(2);
    const expr* suc_rv3 = saved_expr_pool_.make("suc", {rv3});
    const expr* suc_rv5 = saved_expr_pool_.make("suc", {rv5});
    const expr* add_suc = saved_expr_pool_.make("add", {suc_rv3, rv4, suc_rv5});
    const expr* add_body = saved_expr_pool_.make("add", {rv3, rv4, rv5});
    database.push(rule{add_suc, {add_body}});

    const expr* rv6 = saved_expr_pool_.make(0);
    const expr* mul_zero_y_zero = saved_expr_pool_.make("mul", {zero, rv6, zero});
    const expr* nat_rv6 = saved_expr_pool_.make("nat", {rv6});
    database.push(rule{mul_zero_y_zero, {nat_rv6}});

    const expr* rv7 = saved_expr_pool_.make(0);
    const expr* rv8 = saved_expr_pool_.make(1);
    const expr* rv9 = saved_expr_pool_.make(2);
    const expr* rv10 = saved_expr_pool_.make(3);
    const expr* suc_rv7 = saved_expr_pool_.make("suc", {rv7});
    const expr* mul_suc = saved_expr_pool_.make("mul", {suc_rv7, rv8, rv9});
    const expr* mul_body = saved_expr_pool_.make("mul", {rv7, rv8, rv10});
    const expr* add_body2 = saved_expr_pool_.make("add", {rv10, rv8, rv9});
    database.push(rule{mul_suc, {mul_body, add_body2}});

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
        expr_printer_.printer,
        manifest.decision_memory_,
        manifest.resolution_memory_,
        expected,
        [&]() -> solution {
            return {
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_x))),
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_y))),
            };
        });
}

TEST_F(BasicManifestIntegrationTest, EnumeratesDualBoundedSharedXSums) {
    /*
     * Intent: enumerate all triples (X,Y,Z) with add(X,Y,S), add(X,Z,T), lt(S,4), lt(T,4).
     * Ported from legacy integration test 18.
     */
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make("zero", {});
    const expr* nat_zero = saved_expr_pool_.make("nat", {zero});
    database.push(rule{nat_zero, {}});

    const expr* rv1 = saved_expr_pool_.make(0);
    const expr* suc_rv1 = saved_expr_pool_.make("suc", {rv1});
    const expr* nat_suc = saved_expr_pool_.make("nat", {suc_rv1});
    const expr* nat_rv1 = saved_expr_pool_.make("nat", {rv1});
    database.push(rule{nat_suc, {nat_rv1}});

    const expr* rv2 = saved_expr_pool_.make(0);
    const expr* add_zero_y_y = saved_expr_pool_.make("add", {zero, rv2, rv2});
    const expr* nat_rv2 = saved_expr_pool_.make("nat", {rv2});
    database.push(rule{add_zero_y_y, {nat_rv2}});

    const expr* rv3 = saved_expr_pool_.make(0);
    const expr* rv4 = saved_expr_pool_.make(1);
    const expr* rv5 = saved_expr_pool_.make(2);
    const expr* suc_rv3 = saved_expr_pool_.make("suc", {rv3});
    const expr* suc_rv5 = saved_expr_pool_.make("suc", {rv5});
    const expr* add_suc = saved_expr_pool_.make("add", {suc_rv3, rv4, suc_rv5});
    const expr* add_body = saved_expr_pool_.make("add", {rv3, rv4, rv5});
    database.push(rule{add_suc, {add_body}});

    const expr* rv6 = saved_expr_pool_.make(0);
    const expr* suc_rv6 = saved_expr_pool_.make("suc", {rv6});
    const expr* lt_zero_suc = saved_expr_pool_.make("lt", {zero, suc_rv6});
    const expr* nat_rv6 = saved_expr_pool_.make("nat", {rv6});
    database.push(rule{lt_zero_suc, {nat_rv6}});

    const expr* rv7 = saved_expr_pool_.make(0);
    const expr* rv8 = saved_expr_pool_.make(1);
    const expr* suc_rv7 = saved_expr_pool_.make("suc", {rv7});
    const expr* suc_rv8 = saved_expr_pool_.make("suc", {rv8});
    const expr* lt_suc_suc = saved_expr_pool_.make("lt", {suc_rv7, suc_rv8});
    const expr* lt_body = saved_expr_pool_.make("lt", {rv7, rv8});
    database.push(rule{lt_suc_suc, {lt_body}});

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
        expr_printer_.printer,
        manifest.decision_memory_,
        manifest.resolution_memory_,
        expected,
        [&]() -> solution {
            return {
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_x))),
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_y))),
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_z))),
            };
        });
}

TEST_F(BasicManifestIntegrationTest, EnumeratesCatalanTreesWithFiveNodes) {
    /*
     * Intent: enumerate all wf(T) trees with exactly five nodes (Catalan C5 = 42).
     * Ported from legacy integration test 19.
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

    const expr* zero = saved_expr_pool_.make("zero", {});
    const expr* nat_zero = saved_expr_pool_.make("nat", {zero});
    database.push(rule{nat_zero, {}});

    const expr* rv1 = saved_expr_pool_.make(0);
    const expr* suc_rv1 = saved_expr_pool_.make("suc", {rv1});
    const expr* nat_suc = saved_expr_pool_.make("nat", {suc_rv1});
    const expr* nat_rv1 = saved_expr_pool_.make("nat", {rv1});
    database.push(rule{nat_suc, {nat_rv1}});

    const expr* rv2 = saved_expr_pool_.make(0);
    const expr* add_zero_y_y = saved_expr_pool_.make("add", {zero, rv2, rv2});
    const expr* nat_rv2 = saved_expr_pool_.make("nat", {rv2});
    database.push(rule{add_zero_y_y, {nat_rv2}});

    const expr* rv3 = saved_expr_pool_.make(0);
    const expr* rv4 = saved_expr_pool_.make(1);
    const expr* rv5 = saved_expr_pool_.make(2);
    const expr* suc_rv3 = saved_expr_pool_.make("suc", {rv3});
    const expr* suc_rv5 = saved_expr_pool_.make("suc", {rv5});
    const expr* add_suc = saved_expr_pool_.make("add", {suc_rv3, rv4, suc_rv5});
    const expr* add_body = saved_expr_pool_.make("add", {rv3, rv4, rv5});
    database.push(rule{add_suc, {add_body}});

    const expr* nil = saved_expr_pool_.make("nil", {});
    const expr* wf_nil = saved_expr_pool_.make("wf", {nil});
    database.push(rule{wf_nil, {}});

    const expr* rv6 = saved_expr_pool_.make(0);
    const expr* rv7 = saved_expr_pool_.make(1);
    const expr* bin_rv6_rv7 = saved_expr_pool_.make("bin", {rv6, rv7});
    const expr* wf_bin = saved_expr_pool_.make("wf", {bin_rv6_rv7});
    const expr* wf_left = saved_expr_pool_.make("wf", {rv6});
    const expr* wf_right = saved_expr_pool_.make("wf", {rv7});
    database.push(rule{wf_bin, {wf_left, wf_right}});

    const expr* nodes_nil_zero = saved_expr_pool_.make("nodes", {nil, zero});
    database.push(rule{nodes_nil_zero, {}});

    const expr* one = saved_expr_pool_.make("suc", {zero});
    const expr* rv8 = saved_expr_pool_.make(0);
    const expr* rv9 = saved_expr_pool_.make(1);
    const expr* rv10 = saved_expr_pool_.make(2);
    const expr* rv11 = saved_expr_pool_.make(3);
    const expr* rv12 = saved_expr_pool_.make(4);
    const expr* rv13 = saved_expr_pool_.make(5);
    const expr* bin_rv8_rv9 = saved_expr_pool_.make("bin", {rv8, rv9});
    const expr* nodes_head = saved_expr_pool_.make("nodes", {bin_rv8_rv9, rv10});
    const expr* nodes_left = saved_expr_pool_.make("nodes", {rv8, rv11});
    const expr* nodes_right = saved_expr_pool_.make("nodes", {rv9, rv12});
    const expr* add_sizes = saved_expr_pool_.make("add", {rv11, rv12, rv13});
    const expr* add_one = saved_expr_pool_.make("add", {one, rv13, rv10});
    database.push(rule{nodes_head, {nodes_left, nodes_right, add_sizes, add_one}});

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
        expr_printer_.printer,
        manifest.decision_memory_,
        manifest.resolution_memory_,
        expected,
        [&]() -> solution {
            return {
                saved_expr_pool_.import(normalizer.normalize(manifest.expr_pool_.make(idx_t))),
            };
        });
}
