// Integration-style tests for basic_solver_session — real wiring, no mocks.
//
// solved() is only valid immediately after next() returned true.
// Typical usage: while (session.next()) { if (session.solved()) { ... } }

#include <optional>
#include <set>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/basic_manifest.hpp"
#include "infrastructure/basic_solver_session.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/locator.hpp"
#include "infrastructure/trail.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/sim_termination.hpp"

using solution = std::vector<const expr*>;

struct BasicSolverSessionTest : public ::testing::Test {
    static constexpr size_t kMaxResolutions = 32;
    static constexpr uint32_t kSeed = 42;

    db database;
    initial_goal_exprs initial_goals;

    trail saved_trail_;
    locator saved_loc_;
    expr_pool saved_expr_pool_{
        (saved_loc_.bind_as<i_log_to_current_trail_frame>(saved_trail_), saved_loc_)};
};

// Tier A — session API smoke

TEST_F(BasicSolverSessionTest, ConstructsWithEmptyDbAndGoals) {
    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_F(BasicSolverSessionTest, VacuousSolvedTick) {
    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_TRUE(session.derive_decision_lemma().get_resolutions().empty());
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_F(BasicSolverSessionTest, NormalizeDelegatesToBindMap) {
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* _123 = saved_expr_pool_.make("123", {});
    database.push(rule{saved_expr_pool_.make("f", {abc, _123}), {}});

    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    initial_goals.push(saved_expr_pool_.make("f", {
        saved_expr_pool_.make(idx_a),
        saved_expr_pool_.make(idx_b),
    }));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_EQ(saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a))),
        abc);
    EXPECT_EQ(saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_b))),
        _123);
    EXPECT_FALSE(session.next()) << "expected refutation";
    EXPECT_TRUE(std::holds_alternative<expr::var>(
        session.normalize(saved_expr_pool_.make(idx_a))->content));
}

TEST_F(BasicSolverSessionTest, ImportSurvivesNextTick) {
    const expr* abc = saved_expr_pool_.make("abc", {});
    database.push(rule{saved_expr_pool_.make("f", {abc}), {}});

    constexpr uint32_t idx_a = 0;
    initial_goals.push(saved_expr_pool_.make("f", {saved_expr_pool_.make(idx_a)}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    const expr* kept = nullptr;
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    kept = saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a)));
    EXPECT_EQ(*kept, *abc);
    EXPECT_FALSE(session.next()) << "expected refutation";
    EXPECT_EQ(*kept, *abc);
}

TEST_F(BasicSolverSessionTest, DeriveDecisionLemmaOnDemand) {
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* head0 = saved_expr_pool_.make("f", {});
    const expr* head1 = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
    database.push(rule{head0, {}});
    database.push(rule{head1, {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_FALSE(session.derive_decision_lemma().get_resolutions().empty());
}

TEST_F(BasicSolverSessionTest, DeriveResolutionLemmaOnDemand) {
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* head = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
    database.push(rule{head, {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_FALSE(session.derive_resolution_lemma().get_resolutions().empty());
}

TEST_F(BasicSolverSessionTest, LemmaNotCachedAcrossTicks) {
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* head0 = saved_expr_pool_.make("f", {});
    const expr* head1 = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
    database.push(rule{head0, {}});
    database.push(rule{head1, {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);

    std::optional<lemma> decision1;
    std::optional<lemma> resolution1;
    size_t solved_ticks = 0;

    ASSERT_TRUE(session.next());
    ASSERT_TRUE(session.solved());
    decision1 = session.derive_decision_lemma();
    resolution1 = session.derive_resolution_lemma();
    EXPECT_EQ(decision1->get_resolutions().size(), 1u);
    ++solved_ticks;

    ASSERT_TRUE(session.next());
    ASSERT_TRUE(session.solved());
    const lemma decision2 = session.derive_decision_lemma();
    const lemma resolution2 = session.derive_resolution_lemma();
    EXPECT_TRUE(decision2.get_resolutions().empty());
    EXPECT_FALSE(decision1->get_resolutions() == decision2.get_resolutions()
        && resolution1->get_resolutions() == resolution2.get_resolutions());
    ++solved_ticks;

    ASSERT_EQ(solved_ticks, 2u);
    EXPECT_FALSE(session.next()) << "expected refutation";
}

// Tier B — solver outcomes

TEST_F(BasicSolverSessionTest, FindsSingleUnitSolution) {
    const expr* goal = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
    database.push(rule{saved_expr_pool_.make("f", {}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_TRUE(session.derive_decision_lemma().get_resolutions().empty());
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_F(BasicSolverSessionTest, RefutesWhenNoCandidates) {
    initial_goals.push(saved_expr_pool_.make("f", {}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next()) << "expected a tick";
    EXPECT_FALSE(session.solved()) << "expected conflict termination";
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_F(BasicSolverSessionTest, FindsClauseDerivedUnitSolution) {
    const expr* g_body = saved_expr_pool_.make("g", {});
    initial_goals.push(saved_expr_pool_.make("f", {}));
    database.push(rule{saved_expr_pool_.make("f", {}), {g_body}});
    database.push(rule{saved_expr_pool_.make("g", {}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_TRUE(session.derive_decision_lemma().get_resolutions().empty());
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_F(BasicSolverSessionTest, FindsSolutionWithCorrectBindings) {
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* _123 = saved_expr_pool_.make("123", {});
    database.push(rule{saved_expr_pool_.make("f", {abc, _123}), {}});

    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    initial_goals.push(saved_expr_pool_.make("f", {
        saved_expr_pool_.make(idx_a),
        saved_expr_pool_.make(idx_b),
    }));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_EQ(*saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a))),
        *abc);
    EXPECT_EQ(*saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_b))),
        *_123);
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_F(BasicSolverSessionTest, FindsClauseBodyBindingSolution) {
    const expr* rule_var_a = saved_expr_pool_.make(0);
    const expr* rule_var_b = saved_expr_pool_.make(1);
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* _123 = saved_expr_pool_.make("123", {});
    database.push(rule{
        saved_expr_pool_.make("f", {rule_var_a, rule_var_b}),
        {saved_expr_pool_.make("g", {rule_var_a}), saved_expr_pool_.make("h", {rule_var_b})}});
    database.push(rule{saved_expr_pool_.make("g", {abc}), {}});
    database.push(rule{saved_expr_pool_.make("h", {_123}), {}});

    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    initial_goals.push(saved_expr_pool_.make("f", {
        saved_expr_pool_.make(idx_a),
        saved_expr_pool_.make(idx_b),
    }));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_EQ(*saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a))),
        *abc);
    EXPECT_EQ(*saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_b))),
        *_123);
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_F(BasicSolverSessionTest, RefutesAfterCdclOnUnsatClauseBranches) {
    const expr* b = saved_expr_pool_.make("b", {});
    const expr* c = saved_expr_pool_.make("c", {});
    database.push(rule{saved_expr_pool_.make("a", {}), {b}});
    database.push(rule{saved_expr_pool_.make("a", {}), {c}});
    initial_goals.push(saved_expr_pool_.make("a", {}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    size_t conflict_ticks = 0;
    while (session.next()) {
        EXPECT_FALSE(session.solved());
        ++conflict_ticks;
    }
    ASSERT_GE(conflict_ticks, 2u);
}

// Tier C — enumeration

TEST_F(BasicSolverSessionTest, EnumeratesTwoGroundChoiceSolutions) {
    initial_goals.push(saved_expr_pool_.make("f", {}));
    database.push(rule{saved_expr_pool_.make("f", {}), {}});
    database.push(rule{saved_expr_pool_.make("f", {}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 2u);
}

TEST_F(BasicSolverSessionTest, RefutesAfterEnumeratingAllGroundBranches) {
    initial_goals.push(saved_expr_pool_.make("f", {}));
    database.push(rule{saved_expr_pool_.make("f", {}), {}});
    database.push(rule{saved_expr_pool_.make("f", {}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 2u);
}

TEST_F(BasicSolverSessionTest, EnumeratesTwoVarChoiceSolutions) {
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* xyz = saved_expr_pool_.make("xyz", {});
    database.push(rule{saved_expr_pool_.make("f", {abc}), {}});
    database.push(rule{saved_expr_pool_.make("f", {xyz}), {}});

    constexpr uint32_t idx_a = 0;
    initial_goals.push(saved_expr_pool_.make("f", {saved_expr_pool_.make(idx_a)}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<solution> expected{{abc}, {xyz}};
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a)))};
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver refuted before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, RefutesAfterEnumeratingAllVarBranches) {
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* xyz = saved_expr_pool_.make("xyz", {});
    database.push(rule{saved_expr_pool_.make("f", {abc}), {}});
    database.push(rule{saved_expr_pool_.make("f", {xyz}), {}});

    constexpr uint32_t idx_a = 0;
    initial_goals.push(saved_expr_pool_.make("f", {saved_expr_pool_.make(idx_a)}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<solution> expected{{abc}, {xyz}};
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a)))};
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver refuted before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesTwoGoalSharedVarSolutions) {
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* xyz = saved_expr_pool_.make("xyz", {});
    database.push(rule{saved_expr_pool_.make("f", {abc}), {}});
    database.push(rule{saved_expr_pool_.make("f", {xyz}), {}});
    database.push(rule{saved_expr_pool_.make("g", {abc}), {}});
    database.push(rule{saved_expr_pool_.make("g", {xyz}), {}});

    constexpr uint32_t idx_a = 0;
    const expr* goal_var = saved_expr_pool_.make(idx_a);
    initial_goals.push(saved_expr_pool_.make("f", {goal_var}));
    initial_goals.push(saved_expr_pool_.make("g", {goal_var}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<solution> expected{{abc}, {xyz}};
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a)))};
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver refuted before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesFourVarBindingSolutions) {
    const expr* a = saved_expr_pool_.make("a", {});
    const expr* b = saved_expr_pool_.make("b", {});
    const expr* c = saved_expr_pool_.make("c", {});
    const expr* d = saved_expr_pool_.make("d", {});
    database.push(rule{saved_expr_pool_.make("f", {a}), {}});
    database.push(rule{saved_expr_pool_.make("f", {b}), {}});
    database.push(rule{saved_expr_pool_.make("f", {c}), {}});
    database.push(rule{saved_expr_pool_.make("f", {d}), {}});

    constexpr uint32_t idx = 0;
    initial_goals.push(saved_expr_pool_.make("f", {saved_expr_pool_.make(idx)}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<solution> expected{{a}, {b}, {c}, {d}};
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx)))};
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver refuted before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesFourClauseBodyFactChoices) {
    const expr* rule_var = saved_expr_pool_.make(0);
    const expr* a = saved_expr_pool_.make("a", {});
    const expr* b = saved_expr_pool_.make("b", {});
    const expr* c = saved_expr_pool_.make("c", {});
    const expr* d = saved_expr_pool_.make("d", {});
    database.push(rule{
        saved_expr_pool_.make("f", {}),
        {saved_expr_pool_.make("g", {rule_var})}});
    database.push(rule{saved_expr_pool_.make("g", {a}), {}});
    database.push(rule{saved_expr_pool_.make("g", {b}), {}});
    database.push(rule{saved_expr_pool_.make("g", {c}), {}});
    database.push(rule{saved_expr_pool_.make("g", {d}), {}});
    initial_goals.push(saved_expr_pool_.make("f", {}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 4u);
}

// Tier E

TEST_F(BasicSolverSessionTest, FindsUniqueSharedVarConjunctionThenRefutes) {
    const expr* one = saved_expr_pool_.make("1", {});
    const expr* two = saved_expr_pool_.make("2", {});
    const expr* three = saved_expr_pool_.make("3", {});
    database.push(rule{saved_expr_pool_.make("is_a", {one}), {}});
    database.push(rule{saved_expr_pool_.make("is_a", {two}), {}});
    database.push(rule{saved_expr_pool_.make("is_b", {two}), {}});
    database.push(rule{saved_expr_pool_.make("is_b", {three}), {}});

    constexpr uint32_t idx_x = 0;
    const expr* goal_var = saved_expr_pool_.make(idx_x);
    initial_goals.push(saved_expr_pool_.make("is_a", {goal_var}));
    initial_goals.push(saved_expr_pool_.make("is_b", {goal_var}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<solution> expected{{two}};
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_x)))};
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver refuted before all expected solutions found";
}

// Tier F

TEST_F(BasicSolverSessionTest, ConflictedTickNotSolved) {
    initial_goals.push(saved_expr_pool_.make("f", {}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next()) << "expected a tick";
    EXPECT_FALSE(session.solved()) << "expected conflict termination";
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_F(BasicSolverSessionTest, SkippedLemmaCallsOnUnitTicks) {
    initial_goals.push(saved_expr_pool_.make("f", {}));
    database.push(rule{saved_expr_pool_.make("f", {}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_FALSE(session.next()) << "expected refutation";
}

// Tier G — CHC enumeration via basic_solver_session

TEST_F(BasicSolverSessionTest, EnumeratesTwoParentBindingsForAlice) {
    const expr* bob = saved_expr_pool_.make("bob", {});
    const expr* carol = saved_expr_pool_.make("carol", {});
    const expr* alice = saved_expr_pool_.make("alice", {});
    const expr* dave = saved_expr_pool_.make("dave", {});
    database.push(rule{saved_expr_pool_.make("parent", {bob, alice}), {}});
    database.push(rule{saved_expr_pool_.make("parent", {carol, alice}), {}});
    database.push(rule{saved_expr_pool_.make("parent", {dave, bob}), {}});

    basic_manifest probe{database, initial_goals, kMaxResolutions, kSeed};
    const uint32_t idx_x = probe.var_sequencer_.next();
    initial_goals.push(probe.expr_pool_.make("parent", {
        probe.expr_pool_.make(idx_x),
        probe.expr_pool_.make("alice", {}),
    }));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<solution> expected{{bob}, {carol}};
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_x)))};
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver stopped before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesPeanoLessThanSeven) {
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make("zero", {});
    database.push(rule{saved_expr_pool_.make("nat", {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("nat", {saved_expr_pool_.make("suc", {rv1})}),
        {saved_expr_pool_.make("nat", {rv1})}});

    const expr* rv2 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("lt", {zero, saved_expr_pool_.make("suc", {rv2})}),
        {saved_expr_pool_.make("nat", {rv2})}});

    const expr* rv3 = saved_expr_pool_.make(0);
    const expr* rv4 = saved_expr_pool_.make(1);
    database.push(rule{
        saved_expr_pool_.make("lt", {saved_expr_pool_.make("suc", {rv3}), saved_expr_pool_.make("suc", {rv4})}),
        {saved_expr_pool_.make("lt", {rv3, rv4})}});

    std::set<solution> expected;
    for (int n = 0; n < 7; ++n)
        expected.insert({peano_saved(n)});

    initial_goal_exprs probe_goals;
    basic_manifest probe{database, probe_goals, kPeanoBudget, kSeed};
    const uint32_t idx_n = probe.var_sequencer_.next();
    const expr* seven = probe.expr_pool_.make("zero", {});
    for (int i = 0; i < 7; ++i)
        seven = probe.expr_pool_.make("suc", {seven});
    initial_goals.push(probe.expr_pool_.make("nat", {probe.expr_pool_.make(idx_n)}));
    initial_goals.push(probe.expr_pool_.make("lt", {
        probe.expr_pool_.make(idx_n),
        seven,
    }));

    basic_solver_session session(database, initial_goals, kPeanoBudget, kSeed);
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_n)))};
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver stopped before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesSatPAndQOrR) {
    const expr* true_atom = saved_expr_pool_.make("true", {});
    const expr* false_atom = saved_expr_pool_.make("false", {});
    database.push(rule{saved_expr_pool_.make("bool", {true_atom}), {}});
    database.push(rule{saved_expr_pool_.make("bool", {false_atom}), {}});

    const expr* or_rv = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("or", {true_atom, or_rv, true_atom}),
        {saved_expr_pool_.make("bool", {or_rv})}});

    const expr* or_rv2 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("or", {false_atom, or_rv2, or_rv2}),
        {saved_expr_pool_.make("bool", {or_rv2})}});

    const expr* and_rv = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("and", {true_atom, and_rv, and_rv}),
        {saved_expr_pool_.make("bool", {and_rv})}});

    const expr* and_rv2 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("and", {false_atom, and_rv2, false_atom}),
        {saved_expr_pool_.make("bool", {and_rv2})}});

    basic_manifest probe{database, initial_goals, kMaxResolutions, kSeed};
    const uint32_t idx_p = probe.var_sequencer_.next();
    const uint32_t idx_q = probe.var_sequencer_.next();
    const uint32_t idx_r = probe.var_sequencer_.next();
    const uint32_t idx_qr = probe.var_sequencer_.next();
    initial_goals.push(probe.expr_pool_.make("bool", {probe.expr_pool_.make(idx_p)}));
    initial_goals.push(probe.expr_pool_.make("bool", {probe.expr_pool_.make(idx_q)}));
    initial_goals.push(probe.expr_pool_.make("bool", {probe.expr_pool_.make(idx_r)}));
    initial_goals.push(probe.expr_pool_.make("or", {
        probe.expr_pool_.make(idx_q),
        probe.expr_pool_.make(idx_r),
        probe.expr_pool_.make(idx_qr),
    }));
    initial_goals.push(probe.expr_pool_.make("and", {
        probe.expr_pool_.make(idx_p),
        probe.expr_pool_.make(idx_qr),
        probe.expr_pool_.make("true", {}),
    }));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<solution> expected{
        {true_atom, true_atom, true_atom},
        {true_atom, true_atom, false_atom},
        {true_atom, false_atom, true_atom},
    };
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_p))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_q))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_r))),
        };
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver stopped before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesTwoSatAssignmentsForImpliesQ) {
    const expr* true_atom = saved_expr_pool_.make("true", {});
    const expr* false_atom = saved_expr_pool_.make("false", {});
    database.push(rule{saved_expr_pool_.make("bool", {true_atom}), {}});
    database.push(rule{saved_expr_pool_.make("bool", {false_atom}), {}});
    database.push(rule{saved_expr_pool_.make("not", {true_atom, false_atom}), {}});
    database.push(rule{saved_expr_pool_.make("not", {false_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make("or", {true_atom, true_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make("or", {true_atom, false_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make("or", {false_atom, true_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make("or", {false_atom, false_atom, false_atom}), {}});

    basic_manifest probe{database, initial_goals, kMaxResolutions, kSeed};
    const uint32_t idx_p = probe.var_sequencer_.next();
    const uint32_t idx_q = probe.var_sequencer_.next();
    const uint32_t idx_np = probe.var_sequencer_.next();
    initial_goals.push(probe.expr_pool_.make("bool", {probe.expr_pool_.make(idx_p)}));
    initial_goals.push(probe.expr_pool_.make("bool", {probe.expr_pool_.make(idx_q)}));
    initial_goals.push(probe.expr_pool_.make("or", {
        probe.expr_pool_.make(idx_p),
        probe.expr_pool_.make(idx_q),
        probe.expr_pool_.make("true", {}),
    }));
    initial_goals.push(probe.expr_pool_.make("not", {
        probe.expr_pool_.make(idx_p),
        probe.expr_pool_.make(idx_np),
    }));
    initial_goals.push(probe.expr_pool_.make("or", {
        probe.expr_pool_.make(idx_np),
        probe.expr_pool_.make(idx_q),
        probe.expr_pool_.make("true", {}),
    }));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<std::string> p_values;
    size_t solution_count = 0;
    while (session.next()) {
        if (!session.solved())
            continue;
        ++solution_count;
        const expr* q_val = session.normalize(saved_expr_pool_.make(idx_q));
        const expr* p_val = session.normalize(saved_expr_pool_.make(idx_p));
        ASSERT_TRUE(std::holds_alternative<expr::functor>(q_val->content));
        ASSERT_TRUE(std::holds_alternative<expr::functor>(p_val->content));
        EXPECT_EQ(std::get<expr::functor>(q_val->content).name, "true");
        p_values.insert(std::get<expr::functor>(p_val->content).name);
        if (solution_count >= 2u)
            break;
    }
    EXPECT_EQ(solution_count, 2u);
    EXPECT_EQ(p_values.size(), 2u);
}

TEST_F(BasicSolverSessionTest, EnumeratesTwoPathTwoColorings) {
    const expr* red = saved_expr_pool_.make("red", {});
    const expr* blue = saved_expr_pool_.make("blue", {});
    database.push(rule{saved_expr_pool_.make("color", {red}), {}});
    database.push(rule{saved_expr_pool_.make("color", {blue}), {}});
    database.push(rule{saved_expr_pool_.make("diff", {red, blue}), {}});
    database.push(rule{saved_expr_pool_.make("diff", {blue, red}), {}});

    basic_manifest probe{database, initial_goals, kMaxResolutions, kSeed};
    const uint32_t idx_a = probe.var_sequencer_.next();
    const uint32_t idx_b = probe.var_sequencer_.next();
    const uint32_t idx_c = probe.var_sequencer_.next();
    initial_goals.push(probe.expr_pool_.make("color", {probe.expr_pool_.make(idx_a)}));
    initial_goals.push(probe.expr_pool_.make("color", {probe.expr_pool_.make(idx_b)}));
    initial_goals.push(probe.expr_pool_.make("color", {probe.expr_pool_.make(idx_c)}));
    initial_goals.push(probe.expr_pool_.make("diff", {
        probe.expr_pool_.make(idx_a),
        probe.expr_pool_.make(idx_b),
    }));
    initial_goals.push(probe.expr_pool_.make("diff", {
        probe.expr_pool_.make(idx_b),
        probe.expr_pool_.make(idx_c),
    }));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    auto is_valid_color = [](const std::string& s) {
        return s == "red" || s == "blue";
    };

    std::string a1;
    size_t found = 0;
    while (session.next()) {
        if (!session.solved())
            continue;
        const std::string a_str =
            std::get<expr::functor>(session.normalize(saved_expr_pool_.make(idx_a))->content).name;
        const std::string b_str =
            std::get<expr::functor>(session.normalize(saved_expr_pool_.make(idx_b))->content).name;
        const std::string c_str =
            std::get<expr::functor>(session.normalize(saved_expr_pool_.make(idx_c))->content).name;
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
        if (found >= 2u)
            break;
    }
    EXPECT_EQ(found, 2u);
}
TEST_F(BasicSolverSessionTest, EnumeratesK3ThreeColorings) {
    static constexpr size_t kColorBudget = 128;

    const expr* red = saved_expr_pool_.make("red", {});
    const expr* green = saved_expr_pool_.make("green", {});
    const expr* blue = saved_expr_pool_.make("blue", {});
    database.push(rule{saved_expr_pool_.make("color", {red}), {}});
    database.push(rule{saved_expr_pool_.make("color", {green}), {}});
    database.push(rule{saved_expr_pool_.make("color", {blue}), {}});
    database.push(rule{saved_expr_pool_.make("diff", {red, green}), {}});
    database.push(rule{saved_expr_pool_.make("diff", {red, blue}), {}});
    database.push(rule{saved_expr_pool_.make("diff", {green, red}), {}});
    database.push(rule{saved_expr_pool_.make("diff", {green, blue}), {}});
    database.push(rule{saved_expr_pool_.make("diff", {blue, red}), {}});
    database.push(rule{saved_expr_pool_.make("diff", {blue, green}), {}});

    basic_manifest probe{database, initial_goals, kColorBudget, kSeed};
    const uint32_t idx_a = probe.var_sequencer_.next();
    const uint32_t idx_b = probe.var_sequencer_.next();
    const uint32_t idx_c = probe.var_sequencer_.next();
    initial_goals.push(probe.expr_pool_.make("color", {probe.expr_pool_.make(idx_a)}));
    initial_goals.push(probe.expr_pool_.make("color", {probe.expr_pool_.make(idx_b)}));
    initial_goals.push(probe.expr_pool_.make("color", {probe.expr_pool_.make(idx_c)}));
    initial_goals.push(probe.expr_pool_.make("diff", {
        probe.expr_pool_.make(idx_a),
        probe.expr_pool_.make(idx_b),
    }));
    initial_goals.push(probe.expr_pool_.make("diff", {
        probe.expr_pool_.make(idx_a),
        probe.expr_pool_.make(idx_c),
    }));
    initial_goals.push(probe.expr_pool_.make("diff", {
        probe.expr_pool_.make(idx_b),
        probe.expr_pool_.make(idx_c),
    }));

    basic_solver_session session(database, initial_goals, kColorBudget, kSeed);
    std::set<solution> expected = {
        {red, green, blue}, {red, blue, green}, {green, red, blue},
        {green, blue, red}, {blue, red, green}, {blue, green, red},
    };
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_b))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_c))),
        };
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver stopped before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesK3TailFourNodeColorings) {
    static constexpr size_t kColorBudget = 128;

    const expr* red = saved_expr_pool_.make("red", {});
    const expr* green = saved_expr_pool_.make("green", {});
    const expr* blue = saved_expr_pool_.make("blue", {});
    database.push(rule{saved_expr_pool_.make("color", {red}), {}});
    database.push(rule{saved_expr_pool_.make("color", {green}), {}});
    database.push(rule{saved_expr_pool_.make("color", {blue}), {}});
    database.push(rule{saved_expr_pool_.make("diff", {red, green}), {}});
    database.push(rule{saved_expr_pool_.make("diff", {red, blue}), {}});
    database.push(rule{saved_expr_pool_.make("diff", {green, red}), {}});
    database.push(rule{saved_expr_pool_.make("diff", {green, blue}), {}});
    database.push(rule{saved_expr_pool_.make("diff", {blue, red}), {}});
    database.push(rule{saved_expr_pool_.make("diff", {blue, green}), {}});

    basic_manifest probe{database, initial_goals, kColorBudget, kSeed};
    const uint32_t idx_a = probe.var_sequencer_.next();
    const uint32_t idx_b = probe.var_sequencer_.next();
    const uint32_t idx_c = probe.var_sequencer_.next();
    const uint32_t idx_d = probe.var_sequencer_.next();
    initial_goals.push(probe.expr_pool_.make("color", {probe.expr_pool_.make(idx_a)}));
    initial_goals.push(probe.expr_pool_.make("color", {probe.expr_pool_.make(idx_b)}));
    initial_goals.push(probe.expr_pool_.make("color", {probe.expr_pool_.make(idx_c)}));
    initial_goals.push(probe.expr_pool_.make("color", {probe.expr_pool_.make(idx_d)}));
    initial_goals.push(probe.expr_pool_.make("diff", {
        probe.expr_pool_.make(idx_a),
        probe.expr_pool_.make(idx_b),
    }));
    initial_goals.push(probe.expr_pool_.make("diff", {
        probe.expr_pool_.make(idx_a),
        probe.expr_pool_.make(idx_c),
    }));
    initial_goals.push(probe.expr_pool_.make("diff", {
        probe.expr_pool_.make(idx_b),
        probe.expr_pool_.make(idx_c),
    }));
    initial_goals.push(probe.expr_pool_.make("diff", {
        probe.expr_pool_.make(idx_a),
        probe.expr_pool_.make(idx_d),
    }));

    basic_solver_session session(database, initial_goals, kColorBudget, kSeed);
    std::set<solution> expected = {
        {red, green, blue, green}, {red, green, blue, blue}, {red, blue, green, green},
        {red, blue, green, blue}, {green, red, blue, red}, {green, red, blue, blue},
        {green, blue, red, red}, {green, blue, red, blue}, {blue, red, green, red},
        {blue, red, green, green}, {blue, green, red, red}, {blue, green, red, green},
    };
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_b))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_c))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_d))),
        };
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver stopped before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesFourVarSatThreeClauses) {
    static constexpr size_t kSatBudget = 256;

    const expr* true_atom = saved_expr_pool_.make("true", {});
    const expr* false_atom = saved_expr_pool_.make("false", {});
    database.push(rule{saved_expr_pool_.make("bool", {true_atom}), {}});
    database.push(rule{saved_expr_pool_.make("bool", {false_atom}), {}});
    database.push(rule{saved_expr_pool_.make("not", {true_atom, false_atom}), {}});
    database.push(rule{saved_expr_pool_.make("not", {false_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make("or", {true_atom, true_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make("or", {true_atom, false_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make("or", {false_atom, true_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make("or", {false_atom, false_atom, false_atom}), {}});
    database.push(rule{saved_expr_pool_.make("and", {true_atom, true_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make("and", {true_atom, false_atom, false_atom}), {}});
    database.push(rule{saved_expr_pool_.make("and", {false_atom, true_atom, false_atom}), {}});
    database.push(rule{saved_expr_pool_.make("and", {false_atom, false_atom, false_atom}), {}});

    basic_manifest probe{database, initial_goals, kSatBudget, kSeed};
    const uint32_t idx_p = probe.var_sequencer_.next();
    const uint32_t idx_q = probe.var_sequencer_.next();
    const uint32_t idx_r = probe.var_sequencer_.next();
    const uint32_t idx_s = probe.var_sequencer_.next();
    const uint32_t idx_pq = probe.var_sequencer_.next();
    const uint32_t idx_rs = probe.var_sequencer_.next();
    const uint32_t idx_np = probe.var_sequencer_.next();
    const uint32_t idx_nr = probe.var_sequencer_.next();
    const uint32_t idx_npr = probe.var_sequencer_.next();
    const uint32_t idx_pq_rs = probe.var_sequencer_.next();
    initial_goals.push(probe.expr_pool_.make("bool", {probe.expr_pool_.make(idx_p)}));
    initial_goals.push(probe.expr_pool_.make("bool", {probe.expr_pool_.make(idx_q)}));
    initial_goals.push(probe.expr_pool_.make("bool", {probe.expr_pool_.make(idx_r)}));
    initial_goals.push(probe.expr_pool_.make("bool", {probe.expr_pool_.make(idx_s)}));
    initial_goals.push(probe.expr_pool_.make("or", {
        probe.expr_pool_.make(idx_p),
        probe.expr_pool_.make(idx_q),
        probe.expr_pool_.make(idx_pq),
    }));
    initial_goals.push(probe.expr_pool_.make("or", {
        probe.expr_pool_.make(idx_r),
        probe.expr_pool_.make(idx_s),
        probe.expr_pool_.make(idx_rs),
    }));
    initial_goals.push(probe.expr_pool_.make("not", {
        probe.expr_pool_.make(idx_p),
        probe.expr_pool_.make(idx_np),
    }));
    initial_goals.push(probe.expr_pool_.make("not", {
        probe.expr_pool_.make(idx_r),
        probe.expr_pool_.make(idx_nr),
    }));
    initial_goals.push(probe.expr_pool_.make("or", {
        probe.expr_pool_.make(idx_np),
        probe.expr_pool_.make(idx_nr),
        probe.expr_pool_.make(idx_npr),
    }));
    initial_goals.push(probe.expr_pool_.make("and", {
        probe.expr_pool_.make(idx_pq),
        probe.expr_pool_.make(idx_rs),
        probe.expr_pool_.make(idx_pq_rs),
    }));
    initial_goals.push(probe.expr_pool_.make("and", {
        probe.expr_pool_.make(idx_pq_rs),
        probe.expr_pool_.make(idx_npr),
        probe.expr_pool_.make("true", {}),
    }));

    basic_solver_session session(database, initial_goals, kSatBudget, kSeed);
    std::set<solution> expected = {
        {true_atom, true_atom, false_atom, true_atom},
        {true_atom, false_atom, false_atom, true_atom},
        {false_atom, true_atom, true_atom, true_atom},
        {false_atom, true_atom, true_atom, false_atom},
        {false_atom, true_atom, false_atom, true_atom},
    };
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_p))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_q))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_r))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_s))),
        };
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver stopped before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesAddPairsSummingLessThanTen) {
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make("zero", {});
    database.push(rule{saved_expr_pool_.make("nat", {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("nat", {saved_expr_pool_.make("suc", {rv1})}),
        {saved_expr_pool_.make("nat", {rv1})}});

    const expr* rv2 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("add", {zero, rv2, rv2}),
        {saved_expr_pool_.make("nat", {rv2})}});

    const expr* rv3 = saved_expr_pool_.make(0);
    const expr* rv4 = saved_expr_pool_.make(1);
    const expr* rv5 = saved_expr_pool_.make(2);
    database.push(rule{
        saved_expr_pool_.make("add", {saved_expr_pool_.make("suc", {rv3}), rv4, saved_expr_pool_.make("suc", {rv5})}),
        {saved_expr_pool_.make("add", {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("lt", {zero, saved_expr_pool_.make("suc", {rv6})}),
        {saved_expr_pool_.make("nat", {rv6})}});

    const expr* rv7 = saved_expr_pool_.make(0);
    const expr* rv8 = saved_expr_pool_.make(1);
    database.push(rule{
        saved_expr_pool_.make("lt", {saved_expr_pool_.make("suc", {rv7}), saved_expr_pool_.make("suc", {rv8})}),
        {saved_expr_pool_.make("lt", {rv7, rv8})}});

    std::set<solution> expected;
    for (int x = 0; x < 10; ++x) {
        for (int y = 0; y < 10 - x; ++y)
            expected.insert({peano_saved(x), peano_saved(y)});
    }
    ASSERT_EQ(expected.size(), 55u);

    basic_manifest probe{database, initial_goals, kPeanoBudget, kSeed};
    const uint32_t idx_x = probe.var_sequencer_.next();
    const uint32_t idx_y = probe.var_sequencer_.next();
    const uint32_t idx_s = probe.var_sequencer_.next();
    const expr* ten = probe.expr_pool_.make("zero", {});
    for (int i = 0; i < 10; ++i)
        ten = probe.expr_pool_.make("suc", {ten});
    initial_goals.push(probe.expr_pool_.make("add", {
        probe.expr_pool_.make(idx_x),
        probe.expr_pool_.make(idx_y),
        probe.expr_pool_.make(idx_s),
    }));
    initial_goals.push(probe.expr_pool_.make("lt", {
        probe.expr_pool_.make(idx_s),
        ten,
    }));

    basic_solver_session session(database, initial_goals, kPeanoBudget, kSeed);
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_x))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_y))),
        };
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver stopped before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesAddPairsSummingExactlyTen) {
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make("zero", {});
    database.push(rule{saved_expr_pool_.make("nat", {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("nat", {saved_expr_pool_.make("suc", {rv1})}),
        {saved_expr_pool_.make("nat", {rv1})}});

    const expr* rv2 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("add", {zero, rv2, rv2}),
        {saved_expr_pool_.make("nat", {rv2})}});

    const expr* rv3 = saved_expr_pool_.make(0);
    const expr* rv4 = saved_expr_pool_.make(1);
    const expr* rv5 = saved_expr_pool_.make(2);
    database.push(rule{
        saved_expr_pool_.make("add", {saved_expr_pool_.make("suc", {rv3}), rv4, saved_expr_pool_.make("suc", {rv5})}),
        {saved_expr_pool_.make("add", {rv3, rv4, rv5})}});

    std::set<solution> expected;
    for (int x = 0; x <= 10; ++x)
        expected.insert({peano_saved(x), peano_saved(10 - x)});
    ASSERT_EQ(expected.size(), 11u);

    basic_manifest probe{database, initial_goals, kPeanoBudget, kSeed};
    const uint32_t idx_x = probe.var_sequencer_.next();
    const uint32_t idx_y = probe.var_sequencer_.next();
    const expr* ten = probe.expr_pool_.make("zero", {});
    for (int i = 0; i < 10; ++i)
        ten = probe.expr_pool_.make("suc", {ten});
    initial_goals.push(probe.expr_pool_.make("nat", {probe.expr_pool_.make(idx_x)}));
    initial_goals.push(probe.expr_pool_.make("nat", {probe.expr_pool_.make(idx_y)}));
    initial_goals.push(probe.expr_pool_.make("add", {
        probe.expr_pool_.make(idx_x),
        probe.expr_pool_.make(idx_y),
        ten,
    }));

    basic_solver_session session(database, initial_goals, kPeanoBudget, kSeed);
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_x))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_y))),
        };
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver stopped before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesMulPairsProductEight) {
    static constexpr size_t kPeanoBudget = 256;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make("zero", {});
    database.push(rule{saved_expr_pool_.make("nat", {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("nat", {saved_expr_pool_.make("suc", {rv1})}),
        {saved_expr_pool_.make("nat", {rv1})}});

    const expr* rv2 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("add", {zero, rv2, rv2}),
        {saved_expr_pool_.make("nat", {rv2})}});

    const expr* rv3 = saved_expr_pool_.make(0);
    const expr* rv4 = saved_expr_pool_.make(1);
    const expr* rv5 = saved_expr_pool_.make(2);
    database.push(rule{
        saved_expr_pool_.make("add", {saved_expr_pool_.make("suc", {rv3}), rv4, saved_expr_pool_.make("suc", {rv5})}),
        {saved_expr_pool_.make("add", {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("mul", {zero, rv6, zero}),
        {saved_expr_pool_.make("nat", {rv6})}});

    const expr* rv7 = saved_expr_pool_.make(0);
    const expr* rv8 = saved_expr_pool_.make(1);
    const expr* rv9 = saved_expr_pool_.make(2);
    const expr* rv10 = saved_expr_pool_.make(3);
    database.push(rule{
        saved_expr_pool_.make("mul", {saved_expr_pool_.make("suc", {rv7}), rv8, rv9}),
        {saved_expr_pool_.make("mul", {rv7, rv8, rv10}),
            saved_expr_pool_.make("add", {rv10, rv8, rv9})}});

    std::set<solution> expected = {
        {peano_saved(1), peano_saved(8)},
        {peano_saved(2), peano_saved(4)},
        {peano_saved(4), peano_saved(2)},
        {peano_saved(8), peano_saved(1)},
    };

    basic_manifest probe{database, initial_goals, kPeanoBudget, kSeed};
    const uint32_t idx_x = probe.var_sequencer_.next();
    const uint32_t idx_y = probe.var_sequencer_.next();
    const expr* eight = probe.expr_pool_.make("zero", {});
    for (int i = 0; i < 8; ++i)
        eight = probe.expr_pool_.make("suc", {eight});
    initial_goals.push(probe.expr_pool_.make("nat", {probe.expr_pool_.make(idx_x)}));
    initial_goals.push(probe.expr_pool_.make("nat", {probe.expr_pool_.make(idx_y)}));
    initial_goals.push(probe.expr_pool_.make("mul", {
        probe.expr_pool_.make(idx_x),
        probe.expr_pool_.make(idx_y),
        eight,
    }));

    basic_solver_session session(database, initial_goals, kPeanoBudget, kSeed);
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_x))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_y))),
        };
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver stopped before all expected solutions found";
}
TEST_F(BasicSolverSessionTest, EnumeratesDualBoundedSharedXSums) {
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make("zero", {});
    database.push(rule{saved_expr_pool_.make("nat", {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("nat", {saved_expr_pool_.make("suc", {rv1})}),
        {saved_expr_pool_.make("nat", {rv1})}});

    const expr* rv2 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("add", {zero, rv2, rv2}),
        {saved_expr_pool_.make("nat", {rv2})}});

    const expr* rv3 = saved_expr_pool_.make(0);
    const expr* rv4 = saved_expr_pool_.make(1);
    const expr* rv5 = saved_expr_pool_.make(2);
    database.push(rule{
        saved_expr_pool_.make("add", {saved_expr_pool_.make("suc", {rv3}), rv4, saved_expr_pool_.make("suc", {rv5})}),
        {saved_expr_pool_.make("add", {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("lt", {zero, saved_expr_pool_.make("suc", {rv6})}),
        {saved_expr_pool_.make("nat", {rv6})}});

    const expr* rv7 = saved_expr_pool_.make(0);
    const expr* rv8 = saved_expr_pool_.make(1);
    database.push(rule{
        saved_expr_pool_.make("lt", {saved_expr_pool_.make("suc", {rv7}), saved_expr_pool_.make("suc", {rv8})}),
        {saved_expr_pool_.make("lt", {rv7, rv8})}});

    std::set<solution> expected;
    for (int x = 0; x < 4; ++x) {
        for (int y = 0; y < 4 - x; ++y) {
            for (int z = 0; z < 4 - x; ++z)
                expected.insert({peano_saved(x), peano_saved(y), peano_saved(z)});
        }
    }
    ASSERT_EQ(expected.size(), 30u);

    basic_manifest probe{database, initial_goals, kPeanoBudget, kSeed};
    const uint32_t idx_x = probe.var_sequencer_.next();
    const uint32_t idx_y = probe.var_sequencer_.next();
    const uint32_t idx_z = probe.var_sequencer_.next();
    const uint32_t idx_s = probe.var_sequencer_.next();
    const uint32_t idx_t = probe.var_sequencer_.next();
    const expr* bound = probe.expr_pool_.make("zero", {});
    for (int i = 0; i < 4; ++i)
        bound = probe.expr_pool_.make("suc", {bound});
    initial_goals.push(probe.expr_pool_.make("nat", {probe.expr_pool_.make(idx_x)}));
    initial_goals.push(probe.expr_pool_.make("nat", {probe.expr_pool_.make(idx_y)}));
    initial_goals.push(probe.expr_pool_.make("nat", {probe.expr_pool_.make(idx_z)}));
    initial_goals.push(probe.expr_pool_.make("add", {
        probe.expr_pool_.make(idx_x),
        probe.expr_pool_.make(idx_y),
        probe.expr_pool_.make(idx_s),
    }));
    initial_goals.push(probe.expr_pool_.make("add", {
        probe.expr_pool_.make(idx_x),
        probe.expr_pool_.make(idx_z),
        probe.expr_pool_.make(idx_t),
    }));
    initial_goals.push(probe.expr_pool_.make("lt", {
        probe.expr_pool_.make(idx_s),
        bound,
    }));
    initial_goals.push(probe.expr_pool_.make("lt", {
        probe.expr_pool_.make(idx_t),
        bound,
    }));

    basic_solver_session session(database, initial_goals, kPeanoBudget, kSeed);
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_x))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_y))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_z))),
        };
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver stopped before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesCatalanTreesWithFiveNodes) {
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
    database.push(rule{saved_expr_pool_.make("nat", {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("nat", {saved_expr_pool_.make("suc", {rv1})}),
        {saved_expr_pool_.make("nat", {rv1})}});

    const expr* rv2 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("add", {zero, rv2, rv2}),
        {saved_expr_pool_.make("nat", {rv2})}});

    const expr* rv3 = saved_expr_pool_.make(0);
    const expr* rv4 = saved_expr_pool_.make(1);
    const expr* rv5 = saved_expr_pool_.make(2);
    database.push(rule{
        saved_expr_pool_.make("add", {saved_expr_pool_.make("suc", {rv3}), rv4, saved_expr_pool_.make("suc", {rv5})}),
        {saved_expr_pool_.make("add", {rv3, rv4, rv5})}});

    const expr* nil = saved_expr_pool_.make("nil", {});
    database.push(rule{saved_expr_pool_.make("wf", {nil}), {}});

    const expr* rv6 = saved_expr_pool_.make(0);
    const expr* rv7 = saved_expr_pool_.make(1);
    const expr* bin_rv6_rv7 = saved_expr_pool_.make("bin", {rv6, rv7});
    database.push(rule{
        saved_expr_pool_.make("wf", {bin_rv6_rv7}),
        {saved_expr_pool_.make("wf", {rv6}), saved_expr_pool_.make("wf", {rv7})}});

    database.push(rule{saved_expr_pool_.make("nodes", {nil, zero}), {}});

    const expr* one = saved_expr_pool_.make("suc", {zero});
    const expr* rv8 = saved_expr_pool_.make(0);
    const expr* rv9 = saved_expr_pool_.make(1);
    const expr* rv10 = saved_expr_pool_.make(2);
    const expr* rv11 = saved_expr_pool_.make(3);
    const expr* rv12 = saved_expr_pool_.make(4);
    const expr* rv13 = saved_expr_pool_.make(5);
    const expr* bin_rv8_rv9 = saved_expr_pool_.make("bin", {rv8, rv9});
    database.push(rule{
        saved_expr_pool_.make("nodes", {bin_rv8_rv9, rv10}),
        {saved_expr_pool_.make("nodes", {rv8, rv11}),
            saved_expr_pool_.make("nodes", {rv9, rv12}),
            saved_expr_pool_.make("add", {rv11, rv12, rv13}),
            saved_expr_pool_.make("add", {one, rv13, rv10})}});

    basic_manifest probe{database, initial_goals, kCatalanBudget, kSeed};
    const uint32_t idx_t = probe.var_sequencer_.next();
    const expr* five = probe.expr_pool_.make("zero", {});
    for (int i = 0; i < 5; ++i)
        five = probe.expr_pool_.make("suc", {five});
    initial_goals.push(probe.expr_pool_.make("wf", {probe.expr_pool_.make(idx_t)}));
    initial_goals.push(probe.expr_pool_.make("nodes", {probe.expr_pool_.make(idx_t), five}));

    basic_solver_session session(database, initial_goals, kCatalanBudget, kSeed);
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_t)))};
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    EXPECT_EQ(visited.size(), 42u);
    ASSERT_TRUE(expected.empty()) << "solver stopped before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesFourTwoGoalGroundCombinations) {
    initial_goals.push(saved_expr_pool_.make("f", {}));
    initial_goals.push(saved_expr_pool_.make("g", {}));
    database.push(rule{saved_expr_pool_.make("f", {}), {}});
    database.push(rule{saved_expr_pool_.make("f", {}), {}});
    database.push(rule{saved_expr_pool_.make("g", {}), {}});
    database.push(rule{saved_expr_pool_.make("g", {}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 4u);
}

TEST_F(BasicSolverSessionTest, EnumeratesEightThreeGoalGroundCombinations) {
    initial_goals.push(saved_expr_pool_.make("f", {}));
    initial_goals.push(saved_expr_pool_.make("g", {}));
    initial_goals.push(saved_expr_pool_.make("h", {}));
    database.push(rule{saved_expr_pool_.make("f", {}), {}});
    database.push(rule{saved_expr_pool_.make("f", {}), {}});
    database.push(rule{saved_expr_pool_.make("g", {}), {}});
    database.push(rule{saved_expr_pool_.make("g", {}), {}});
    database.push(rule{saved_expr_pool_.make("h", {}), {}});
    database.push(rule{saved_expr_pool_.make("h", {}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 8u);
}

TEST_F(BasicSolverSessionTest, EnumeratesManySharedVarGroundHeads) {
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* def = saved_expr_pool_.make("def", {});
    const expr* ghi = saved_expr_pool_.make("ghi", {});
    const expr* jkl = saved_expr_pool_.make("jkl", {});
    const expr* mno = saved_expr_pool_.make("mno", {});
    const expr* pqr = saved_expr_pool_.make("pqr", {});
    const expr* xyz = saved_expr_pool_.make("xyz", {});
    database.push(rule{saved_expr_pool_.make("f", {}), {}});
    database.push(rule{saved_expr_pool_.make("f", {}), {}});
    database.push(rule{saved_expr_pool_.make("g", {abc, xyz, pqr}), {}});
    database.push(rule{saved_expr_pool_.make("g", {def, xyz, pqr}), {}});
    database.push(rule{saved_expr_pool_.make("g", {ghi, xyz, pqr}), {}});
    database.push(rule{saved_expr_pool_.make("g", {jkl, xyz, pqr}), {}});
    database.push(rule{saved_expr_pool_.make("g", {mno, xyz, pqr}), {}});

    basic_manifest probe{database, initial_goals, kMaxResolutions, kSeed};
    const uint32_t idx_a = probe.var_sequencer_.next();
    const uint32_t idx_b = probe.var_sequencer_.next();
    const uint32_t idx_c = probe.var_sequencer_.next();
    initial_goals.push(probe.expr_pool_.make("f", {}));
    initial_goals.push(probe.expr_pool_.make("g", {
        probe.expr_pool_.make(idx_a),
        probe.expr_pool_.make(idx_b),
        probe.expr_pool_.make(idx_c),
    }));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_b))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_c))),
        };
        if (visited.count(s))
            continue;
        visited.insert(s);
        ASSERT_EQ(*s[1], *xyz);
        ASSERT_EQ(*s[2], *pqr);
    }
    EXPECT_EQ(visited.size(), 5u);
}

TEST_F(BasicSolverSessionTest, EnumeratesThreeGroundBranches) {
    initial_goals.push(saved_expr_pool_.make("f", {}));
    database.push(rule{saved_expr_pool_.make("f", {}), {}});
    database.push(rule{saved_expr_pool_.make("f", {}), {}});
    database.push(rule{saved_expr_pool_.make("f", {}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 3u);
}

TEST_F(BasicSolverSessionTest, SolvesRecursiveClauseTreeWithoutBranching) {
    initial_goals.push(saved_expr_pool_.make("f", {}));
    database.push(rule{saved_expr_pool_.make("f", {}), {
        saved_expr_pool_.make("g", {}),
        saved_expr_pool_.make("h", {}),
    }});
    database.push(rule{saved_expr_pool_.make("g", {}), {
        saved_expr_pool_.make("i", {}),
        saved_expr_pool_.make("j", {}),
    }});
    database.push(rule{saved_expr_pool_.make("h", {}), {
        saved_expr_pool_.make("i", {}),
        saved_expr_pool_.make("j", {}),
    }});
    database.push(rule{saved_expr_pool_.make("i", {}), {}});
    database.push(rule{saved_expr_pool_.make("j", {}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 1u);
}

TEST_F(BasicSolverSessionTest, EnumeratesTransitiveReachFromA) {
    const expr* a = saved_expr_pool_.make("a", {});
    const expr* b = saved_expr_pool_.make("b", {});
    const expr* c = saved_expr_pool_.make("c", {});
    const expr* d = saved_expr_pool_.make("d", {});
    database.push(rule{saved_expr_pool_.make("edge", {a, b}), {}});
    database.push(rule{saved_expr_pool_.make("edge", {b, c}), {}});
    database.push(rule{saved_expr_pool_.make("edge", {c, d}), {}});

    const expr* rv1 = saved_expr_pool_.make(0);
    const expr* rv2 = saved_expr_pool_.make(1);
    database.push(rule{
        saved_expr_pool_.make("reach", {rv1, rv2}),
        {saved_expr_pool_.make("edge", {rv1, rv2})}});

    const expr* rv3 = saved_expr_pool_.make(0);
    const expr* rv4 = saved_expr_pool_.make(1);
    const expr* rv5 = saved_expr_pool_.make(2);
    database.push(rule{
        saved_expr_pool_.make("reach", {rv3, rv5}),
        {saved_expr_pool_.make("reach", {rv3, rv4}), saved_expr_pool_.make("edge", {rv4, rv5})}});
    database.push(rule{saved_expr_pool_.make("f", {}), {}});
    database.push(rule{saved_expr_pool_.make("f", {}), {}});

    basic_manifest probe{database, initial_goals, kMaxResolutions, kSeed};
    const uint32_t idx_y = probe.var_sequencer_.next();
    initial_goals.push(probe.expr_pool_.make("f", {}));
    initial_goals.push(probe.expr_pool_.make("reach", {a, probe.expr_pool_.make(idx_y)}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<solution> expected{{b}, {c}, {d}};
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_y)))};
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver stopped before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesEvenPeanoLessThanEight) {
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make("zero", {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make("suc", {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make("zero", {});
    database.push(rule{saved_expr_pool_.make("nat", {zero}), {}});
    database.push(rule{saved_expr_pool_.make("even", {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("nat", {saved_expr_pool_.make("suc", {rv1})}),
        {saved_expr_pool_.make("nat", {rv1})}});

    const expr* rv2 = saved_expr_pool_.make(0);
    const expr* suc_suc_rv2 = saved_expr_pool_.make("suc", {saved_expr_pool_.make("suc", {rv2})});
    database.push(rule{
        saved_expr_pool_.make("even", {suc_suc_rv2}),
        {saved_expr_pool_.make("even", {rv2})}});

    const expr* rv3 = saved_expr_pool_.make(0);
    database.push(rule{
        saved_expr_pool_.make("lt", {zero, saved_expr_pool_.make("suc", {rv3})}),
        {saved_expr_pool_.make("nat", {rv3})}});

    const expr* rv4 = saved_expr_pool_.make(0);
    const expr* rv5 = saved_expr_pool_.make(1);
    database.push(rule{
        saved_expr_pool_.make("lt", {saved_expr_pool_.make("suc", {rv4}), saved_expr_pool_.make("suc", {rv5})}),
        {saved_expr_pool_.make("lt", {rv4, rv5})}});

    std::set<solution> expected{
        {peano_saved(0)},
        {peano_saved(2)},
        {peano_saved(4)},
        {peano_saved(6)},
    };

    basic_manifest probe{database, initial_goals, kPeanoBudget, kSeed};
    const uint32_t idx_n = probe.var_sequencer_.next();
    const expr* eight = probe.expr_pool_.make("zero", {});
    for (int i = 0; i < 8; ++i)
        eight = probe.expr_pool_.make("suc", {eight});
    initial_goals.push(probe.expr_pool_.make("even", {probe.expr_pool_.make(idx_n)}));
    initial_goals.push(probe.expr_pool_.make("lt", {
        probe.expr_pool_.make(idx_n),
        eight,
    }));

    basic_solver_session session(database, initial_goals, kPeanoBudget, kSeed);
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_n)))};
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver stopped before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesListSplitsForThreeElementList) {
    const expr* nil = saved_expr_pool_.make("nil", {});
    const expr* a = saved_expr_pool_.make("a", {});
    const expr* b = saved_expr_pool_.make("b", {});
    const expr* c = saved_expr_pool_.make("c", {});

    const expr* rv_base = saved_expr_pool_.make(0);
    database.push(rule{saved_expr_pool_.make("append", {nil, rv_base, rv_base}), {}});

    const expr* rv_h = saved_expr_pool_.make(0);
    const expr* rv_t = saved_expr_pool_.make(1);
    const expr* rv_l2 = saved_expr_pool_.make(2);
    const expr* rv_t2 = saved_expr_pool_.make(3);
    const expr* cons_h_t = saved_expr_pool_.make("cons", {rv_h, rv_t});
    const expr* cons_h_t2 = saved_expr_pool_.make("cons", {rv_h, rv_t2});
    database.push(rule{
        saved_expr_pool_.make("append", {cons_h_t, rv_l2, cons_h_t2}),
        {saved_expr_pool_.make("append", {rv_t, rv_l2, rv_t2})}});
    database.push(rule{saved_expr_pool_.make("f", {}), {}});
    database.push(rule{saved_expr_pool_.make("f", {}), {}});

    auto list_saved = [&](std::initializer_list<const expr*> elems) -> const expr* {
        const expr* tail = nil;
        auto it = elems.end();
        while (it != elems.begin()) {
            --it;
            tail = saved_expr_pool_.make("cons", {*it, tail});
        }
        return tail;
    };

    const expr* full_list = list_saved({a, b, c});
    const expr* empty_list = nil;
    const expr* list_a = list_saved({a});
    const expr* list_bc = list_saved({b, c});
    const expr* list_ab = list_saved({a, b});
    const expr* list_c = list_saved({c});
    const expr* list_abc = full_list;

    std::set<solution> expected = {
        {empty_list, list_abc},
        {list_a, list_bc},
        {list_ab, list_c},
        {list_abc, empty_list},
    };

    basic_manifest probe{database, initial_goals, kMaxResolutions, kSeed};
    const uint32_t idx_l1 = probe.var_sequencer_.next();
    const uint32_t idx_l2 = probe.var_sequencer_.next();
    initial_goals.push(probe.expr_pool_.make("f", {}));
    initial_goals.push(probe.expr_pool_.make("append", {
        probe.expr_pool_.make(idx_l1),
        probe.expr_pool_.make(idx_l2),
        full_list,
    }));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_l1))),
            saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_l2))),
        };
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
    }
    ASSERT_TRUE(expected.empty()) << "solver stopped before all expected solutions found";
}

TEST_F(BasicSolverSessionTest, EnumeratesTwoChoiceClauseSolutions) {
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* xyz = saved_expr_pool_.make("xyz", {});
    const expr* rule_var = saved_expr_pool_.make(0);
    initial_goals.push(saved_expr_pool_.make("f", {}));
    database.push(rule{
        saved_expr_pool_.make("f", {}),
        {saved_expr_pool_.make("g", {rule_var})}});
    database.push(rule{saved_expr_pool_.make("g", {abc}), {}});
    database.push(rule{saved_expr_pool_.make("g", {xyz}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 2u);
}
