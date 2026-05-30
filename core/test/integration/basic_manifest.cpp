// Integration: basic_manifest as a whole session object — wiring, sim lifecycle,
// cross-tick solver interop, and multi-cycle search semantics.
//
// Bug policy (docs/testing.md): failing tests indicate suspected production bugs
// unless setup/lifetime/key definitions are wrong. Do not delete or weaken tests.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>
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
#include "interfaces/i_solve.hpp"
#include "interfaces/i_check_active_goals_empty.hpp"
#include "value_objects/sim_termination.hpp"
#include "value_objects/lemma.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

namespace {

template<typename Iface, typename Concrete>
void expect_same_instance(Concrete& concrete, locator& loc) {
    EXPECT_EQ(static_cast<Iface*>(&concrete), &loc.locate<Iface>());
}

struct TickSnapshot {
    std::set<rule_id> decision_rule_ids;
    std::set<rule_id> resolution_rule_ids;
    std::map<uint32_t, const expr*> var_bindings;
};

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

    for (const resolution_lineage* rl : manifest.decision_memory_.derive().get_resolutions())
        snap.decision_rule_ids.insert(rl->idx);

    for (uint32_t idx : tracked_vars) {
        const expr* var = manifest.expr_pool_.make(idx);
        const expr* normalized = normalizer.normalize(var);
        snap.var_bindings[idx] = saved_expr_pool.import(normalized);
    }

    return snap;
}

struct SimTerminationResult {
    sim_termination termination{};
    TickSnapshot snapshot;
};

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

bool var_unbound(basic_manifest& m, uint32_t idx) {
    const expr* whnf = m.bind_map_.whnf(m.expr_pool_.make(idx));
    return std::holds_alternative<expr::var>(whnf->content);
}

struct SolverRun {
    std::vector<sim_termination> terminations;
    std::vector<TickSnapshot> solutions;
    bool completed{};
};

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

bool snapshot_equal(const TickSnapshot& a, const TickSnapshot& b) {
    if (a.decision_rule_ids != b.decision_rule_ids)
        return false;
    if (a.resolution_rule_ids != b.resolution_rule_ids)
        return false;
    if (a.var_bindings.size() != b.var_bindings.size())
        return false;
    for (const auto& [idx, ea] : a.var_bindings) {
        auto it = b.var_bindings.find(idx);
        if (it == b.var_bindings.end() || *ea != *it->second)
            return false;
    }
    return true;
}

void expect_solutions(
    const std::vector<TickSnapshot>& actual,
    std::initializer_list<TickSnapshot> expected) {
    ASSERT_EQ(actual.size(), expected.size());
    std::vector<TickSnapshot> exp_vec(expected);
    std::vector<bool> matched(exp_vec.size(), false);
    for (const TickSnapshot& a : actual) {
        bool found = false;
        for (size_t i = 0; i < exp_vec.size(); ++i) {
            if (!matched[i] && snapshot_equal(a, exp_vec[i])) {
                matched[i] = true;
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "unexpected solution key";
    }
    for (size_t i = 0; i < actual.size(); ++i) {
        for (size_t j = i + 1; j < actual.size(); ++j)
            EXPECT_FALSE(snapshot_equal(actual[i], actual[j])) << "duplicate solution keys";
    }
}

TickSnapshot ground_key(
    std::initializer_list<rule_id> decision_ids,
    std::initializer_list<rule_id> resolution_ids = {}) {
    TickSnapshot s;
    s.decision_rule_ids = std::set<rule_id>(decision_ids);
    s.resolution_rule_ids = std::set<rule_id>(resolution_ids);
    return s;
}

TickSnapshot var_key(
    rule_id decision_id,
    std::map<uint32_t, const expr*> bindings,
    std::initializer_list<rule_id> resolution_ids = {}) {
    TickSnapshot s;
    s.decision_rule_ids = {decision_id};
    s.var_bindings = std::move(bindings);
    s.resolution_rule_ids = std::set<rule_id>(resolution_ids);
    return s;
}

}  // namespace

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
};

// ---------------------------------------------------------------------------
// Tier W — wiring / shared-instance identity (no sim run)
// ---------------------------------------------------------------------------

TEST_F(BasicManifestIntegrationTest, WiringCdclIsLearnAvoidanceNotJoint) {
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_learn_avoidance>(manifest.cdcl_, manifest.loc_);
    EXPECT_NE(static_cast<void*>(&manifest.loc_.locate<i_learn_avoidance>()),
        static_cast<void*>(&manifest.loc_.locate<i_elimination_generator>()));
}

TEST_F(BasicManifestIntegrationTest, WiringJointIsEliminationGeneratorNotCdcl) {
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_elimination_generator>(manifest.joint_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringTrailSharedForPushPopLog) {
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_push_trail_frame>(manifest.trail_, manifest.loc_);
    expect_same_instance<i_pop_trail_frame>(manifest.trail_, manifest.loc_);
    expect_same_instance<i_log_to_current_trail_frame>(manifest.trail_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringDecisionMemorySharedForRecordCountDerive) {
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_record_decision>(manifest.decision_memory_, manifest.loc_);
    expect_same_instance<i_clear_recorded_decisions>(manifest.decision_memory_, manifest.loc_);
    expect_same_instance<i_get_decision_count>(manifest.decision_memory_, manifest.loc_);
    expect_same_instance<i_derive_decision_lemma>(manifest.decision_memory_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringSimSharedForSetUpRunTearDown) {
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_set_up_sim>(manifest.sim_, manifest.loc_);
    expect_same_instance<i_tear_down_sim>(manifest.sim_, manifest.loc_);
    expect_same_instance<i_run_sim>(manifest.sim_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringMhuSharedForHeadOps) {
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_try_add_mhu_head>(manifest.mhu_, manifest.loc_);
    expect_same_instance<i_clear_mhu_heads>(manifest.mhu_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringLineagePoolSharedForMakePinTrim) {
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_make_goal_lineage>(manifest.lineage_pool_, manifest.loc_);
    expect_same_instance<i_make_resolution_lineage>(manifest.lineage_pool_, manifest.loc_);
    expect_same_instance<i_pin_goal_lineage>(manifest.lineage_pool_, manifest.loc_);
    expect_same_instance<i_pin_resolution_lineage>(manifest.lineage_pool_, manifest.loc_);
    expect_same_instance<i_trim_unpinned_lineages>(manifest.lineage_pool_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringSolverIsISolve) {
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    expect_same_instance<i_solve>(manifest.solver_, manifest.loc_);
}

TEST_F(BasicManifestIntegrationTest, WiringConstructsWithEmptyDbAndNoGoals) {
    EXPECT_NO_THROW(
        (basic_manifest{database, initial_goals, kMaxResolutions, kSeed}));
}

// ---------------------------------------------------------------------------
// Tier L — sim lifecycle + subsystems via manifest.sim_
// ---------------------------------------------------------------------------

TEST_F(BasicManifestIntegrationTest, SimLifecycleTrailDepthRestoresAfterEmptyRun) {
    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    const size_t depth_before = manifest.trail_.depth();
    manifest.sim_.set_up();
    EXPECT_EQ(manifest.sim_.run(), sim_termination::solved);
    manifest.sim_.tear_down();
    EXPECT_EQ(manifest.trail_.depth(), depth_before);
}

TEST_F(BasicManifestIntegrationTest, SimLifecycleTrailDepthRestoresAfterConflictedRun) {
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

// ---------------------------------------------------------------------------
// Tier X — single solver tick (cross-component interop)
// ---------------------------------------------------------------------------

TEST_F(BasicManifestIntegrationTest, TickSnapshotBindingsBeforeTearDown) {
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
    EXPECT_TRUE(var_unbound(manifest, idx_a));
    EXPECT_TRUE(var_unbound(manifest, idx_b));
}

TEST_F(BasicManifestIntegrationTest, TickCdclAvoidancesPersistAcrossTearDown) {
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
    expr goal{expr::functor{"f", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    bind_normalizer(manifest);
    auto sm = manifest.solver_.solve();
    const std::optional<SimTerminationResult> tick1 =
        run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
    ASSERT_TRUE(tick1);
    ASSERT_EQ(tick1->termination, sim_termination::solved);
    ASSERT_EQ(tick1->snapshot.decision_rule_ids.size(), 1u);
    const rule_id first = *tick1->snapshot.decision_rule_ids.begin();

    const std::optional<SimTerminationResult> tick2 =
        run_one_tick(manifest, *normalizer_, *saved_expr_pool_, sm);
    ASSERT_TRUE(tick2);
    ASSERT_EQ(tick2->termination, sim_termination::solved);
    ASSERT_EQ(tick2->snapshot.decision_rule_ids.size(), 1u);
    const rule_id second = *tick2->snapshot.decision_rule_ids.begin();
    EXPECT_NE(first, second);
}

// ---------------------------------------------------------------------------
// Tier S — multi-cycle solver (enumeration)
// ---------------------------------------------------------------------------

TEST_F(BasicManifestIntegrationTest, SolverVacuousSolvedOnEmptyProblem) {
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
    expr goal{expr::functor{"f", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    bind_normalizer(manifest);
    const SolverRun run = run_solver(manifest, *normalizer_, *saved_expr_pool_);
    expect_solutions(run.solutions, {ground_key({0}), ground_key({1})});
}

TEST_F(BasicManifestIntegrationTest, SolverRefutesAfterEnumeratingAllGroundBranches) {
    expr goal{expr::functor{"f", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    bind_normalizer(manifest);
    const SolverRun run = run_solver(manifest, *normalizer_, *saved_expr_pool_);
    expect_solutions(run.solutions, {ground_key({0}), ground_key({1})});
    const auto solved_count = std::ranges::count(run.terminations, sim_termination::solved);
    EXPECT_EQ(solved_count, 2);
    EXPECT_TRUE(run.completed);
}

TEST_F(BasicManifestIntegrationTest, SolverFindsClauseDerivedUnitSolution) {
    expr goal{expr::functor{"f", {}}};
    expr rule_var{expr::var{0}};
    expr g_fact{expr::functor{"g", {}}};
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
    expr goal{expr::functor{"f", {}}};
    expr rule_var{expr::var{0}};
    expr g_fact0{expr::functor{"g", {}}};
    expr g_fact1{expr::functor{"g", {}}};
    expr f_head{expr::functor{"f", {}}};
    expr g_body{expr::functor{"g", {&rule_var}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head, {&g_body}});
    database.push(rule{&g_fact0, {}});
    database.push(rule{&g_fact1, {}});

    basic_manifest manifest{database, initial_goals, kMaxResolutions, kSeed};
    bind_normalizer(manifest);
    const SolverRun run = run_solver(manifest, *normalizer_, *saved_expr_pool_);
    expect_solutions(run.solutions, {ground_key({1}), ground_key({2})});
}

TEST_F(BasicManifestIntegrationTest, SolverFindsSolutionWithCorrectBindings) {
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

    const expr* abc_saved = saved_expr_pool_->import(&abc);
    const expr* xyz_saved = saved_expr_pool_->import(&xyz);
    const SolverRun run =
        run_solver(manifest, *normalizer_, *saved_expr_pool_, {idx_a});
    expect_solutions(run.solutions, {
        var_key(0, {{idx_a, abc_saved}}),
        var_key(1, {{idx_a, xyz_saved}}),
    });
}

TEST_F(BasicManifestIntegrationTest, SolverRefutesAfterEnumeratingAllVarBranches) {
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

    const expr* abc_saved = saved_expr_pool_->import(&abc);
    const expr* xyz_saved = saved_expr_pool_->import(&xyz);
    const SolverRun run =
        run_solver(manifest, *normalizer_, *saved_expr_pool_, {idx_a});
    expect_solutions(run.solutions, {
        var_key(0, {{idx_a, abc_saved}}),
        var_key(1, {{idx_a, xyz_saved}}),
    });
    const auto solved_count = std::ranges::count(run.terminations, sim_termination::solved);
    EXPECT_EQ(solved_count, 2);
    EXPECT_TRUE(run.completed);
}

TEST_F(BasicManifestIntegrationTest, SolverEnumeratesTwoGoalSharedVarSolutions) {
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

    const expr* abc_saved = saved_expr_pool_->import(&abc);
    const expr* xyz_saved = saved_expr_pool_->import(&xyz);
    const SolverRun run =
        run_solver(manifest, *normalizer_, *saved_expr_pool_, {idx_a});
    expect_solutions(run.solutions, {
        var_key(0, {{idx_a, abc_saved}}),
        var_key(1, {{idx_a, xyz_saved}}),
    });
}
