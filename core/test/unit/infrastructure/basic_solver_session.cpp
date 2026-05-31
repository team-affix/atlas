// Integration-style tests for basic_solver_session — real wiring, no mocks.
//
// solved() is only valid immediately after next() returned true.
// Harness: enumerate_all_solutions, next_until_refuted — binding enumeration.

#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
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

namespace {

void enumerate_all_solutions(
    basic_solver_session& session,
    std::set<solution> expected,
    const std::function<solution()>& get_solution) {
    std::set<solution> visited;
    while (!expected.empty()) {
        ASSERT_TRUE(session.next()) << "solver stopped before all expected solutions found";
        if (!session.solved())
            continue;
        const solution s = get_solution();
        if (visited.count(s))
            continue;
        auto it = expected.find(s);
        ASSERT_NE(it, expected.end()) << "unexpected solution";
        expected.erase(it);
        visited.insert(s);
    }
}

void next_until_refuted(
    basic_solver_session& session,
    std::set<solution> expected,
    const std::function<solution()>& get_solution) {
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
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
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a)))};
        });
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
    next_until_refuted(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a)))};
        });
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
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a)))};
        });
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
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx)))};
        });
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
    next_until_refuted(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_x)))};
        });
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
    constexpr uint32_t idx_x = 0;
    initial_goals.push(saved_expr_pool_.make("parent", {
        saved_expr_pool_.make(idx_x),
        alice,
    }));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<solution> expected{{bob}, {carol}};
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_x)))};
        });
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
    constexpr uint32_t idx_n = 0;
    const expr* seven = saved_expr_pool_.make("zero", {});
    for (int i = 0; i < 7; ++i)
        seven = saved_expr_pool_.make("suc", {seven});
    initial_goals.push(saved_expr_pool_.make("nat", {saved_expr_pool_.make(idx_n)}));
    initial_goals.push(saved_expr_pool_.make("lt", {
        saved_expr_pool_.make(idx_n),
        seven,
    }));

    basic_solver_session session(database, initial_goals, kPeanoBudget, kSeed);
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_n)))};
        });
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
    constexpr uint32_t idx_p = 0;
    constexpr uint32_t idx_q = 1;
    constexpr uint32_t idx_r = 2;
    constexpr uint32_t idx_qr = 3;
    initial_goals.push(saved_expr_pool_.make("bool", {saved_expr_pool_.make(idx_p)}));
    initial_goals.push(saved_expr_pool_.make("bool", {saved_expr_pool_.make(idx_q)}));
    initial_goals.push(saved_expr_pool_.make("bool", {saved_expr_pool_.make(idx_r)}));
    initial_goals.push(saved_expr_pool_.make("or", {
        saved_expr_pool_.make(idx_q),
        saved_expr_pool_.make(idx_r),
        saved_expr_pool_.make(idx_qr),
    }));
    initial_goals.push(saved_expr_pool_.make("and", {
        saved_expr_pool_.make(idx_p),
        saved_expr_pool_.make(idx_qr),
        saved_expr_pool_.make("true", {}),
    }));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<solution> expected{
        {true_atom, true_atom, true_atom},
        {true_atom, true_atom, false_atom},
        {true_atom, false_atom, true_atom},
    };
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_p))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_q))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_r)))};
        });
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
    constexpr uint32_t idx_p = 0;
    constexpr uint32_t idx_q = 1;
    constexpr uint32_t idx_np = 2;
    initial_goals.push(saved_expr_pool_.make("bool", {saved_expr_pool_.make(idx_p)}));
    initial_goals.push(saved_expr_pool_.make("bool", {saved_expr_pool_.make(idx_q)}));
    initial_goals.push(saved_expr_pool_.make("or", {
        saved_expr_pool_.make(idx_p),
        saved_expr_pool_.make(idx_q),
        saved_expr_pool_.make("true", {}),
    }));
    initial_goals.push(saved_expr_pool_.make("not", {
        saved_expr_pool_.make(idx_p),
        saved_expr_pool_.make(idx_np),
    }));
    initial_goals.push(saved_expr_pool_.make("or", {
        saved_expr_pool_.make(idx_np),
        saved_expr_pool_.make(idx_q),
        saved_expr_pool_.make("true", {}),
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
    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    constexpr uint32_t idx_c = 2;
    initial_goals.push(saved_expr_pool_.make("color", {saved_expr_pool_.make(idx_a)}));
    initial_goals.push(saved_expr_pool_.make("color", {saved_expr_pool_.make(idx_b)}));
    initial_goals.push(saved_expr_pool_.make("color", {saved_expr_pool_.make(idx_c)}));
    initial_goals.push(saved_expr_pool_.make("diff", {
        saved_expr_pool_.make(idx_a),
        saved_expr_pool_.make(idx_b),
    }));
    initial_goals.push(saved_expr_pool_.make("diff", {
        saved_expr_pool_.make(idx_b),
        saved_expr_pool_.make(idx_c),
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
    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    constexpr uint32_t idx_c = 2;
    initial_goals.push(saved_expr_pool_.make("color", {saved_expr_pool_.make(idx_a)}));
    initial_goals.push(saved_expr_pool_.make("color", {saved_expr_pool_.make(idx_b)}));
    initial_goals.push(saved_expr_pool_.make("color", {saved_expr_pool_.make(idx_c)}));
    initial_goals.push(saved_expr_pool_.make("diff", {
        saved_expr_pool_.make(idx_a),
        saved_expr_pool_.make(idx_b),
    }));
    initial_goals.push(saved_expr_pool_.make("diff", {
        saved_expr_pool_.make(idx_a),
        saved_expr_pool_.make(idx_c),
    }));
    initial_goals.push(saved_expr_pool_.make("diff", {
        saved_expr_pool_.make(idx_b),
        saved_expr_pool_.make(idx_c),
    }));

    basic_solver_session session(database, initial_goals, kColorBudget, kSeed);
    std::set<solution> expected = {
        {red, green, blue}, {red, blue, green}, {green, red, blue},
        {green, blue, red}, {blue, red, green}, {blue, green, red},
    };
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_b))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_c)))};
        });
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
    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    constexpr uint32_t idx_c = 2;
    constexpr uint32_t idx_d = 3;
    initial_goals.push(saved_expr_pool_.make("color", {saved_expr_pool_.make(idx_a)}));
    initial_goals.push(saved_expr_pool_.make("color", {saved_expr_pool_.make(idx_b)}));
    initial_goals.push(saved_expr_pool_.make("color", {saved_expr_pool_.make(idx_c)}));
    initial_goals.push(saved_expr_pool_.make("color", {saved_expr_pool_.make(idx_d)}));
    initial_goals.push(saved_expr_pool_.make("diff", {
        saved_expr_pool_.make(idx_a),
        saved_expr_pool_.make(idx_b),
    }));
    initial_goals.push(saved_expr_pool_.make("diff", {
        saved_expr_pool_.make(idx_a),
        saved_expr_pool_.make(idx_c),
    }));
    initial_goals.push(saved_expr_pool_.make("diff", {
        saved_expr_pool_.make(idx_b),
        saved_expr_pool_.make(idx_c),
    }));
    initial_goals.push(saved_expr_pool_.make("diff", {
        saved_expr_pool_.make(idx_a),
        saved_expr_pool_.make(idx_d),
    }));

    basic_solver_session session(database, initial_goals, kColorBudget, kSeed);
    std::set<solution> expected = {
        {red, green, blue, green}, {red, green, blue, blue}, {red, blue, green, green},
        {red, blue, green, blue}, {green, red, blue, red}, {green, red, blue, blue},
        {green, blue, red, red}, {green, blue, red, blue}, {blue, red, green, red},
        {blue, red, green, green}, {blue, green, red, red}, {blue, green, red, green},
    };
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_b))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_c))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_d)))};
        });
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
    constexpr uint32_t idx_p = 0;
    constexpr uint32_t idx_q = 1;
    constexpr uint32_t idx_r = 2;
    constexpr uint32_t idx_s = 3;
    constexpr uint32_t idx_pq = 4;
    constexpr uint32_t idx_rs = 5;
    constexpr uint32_t idx_np = 6;
    constexpr uint32_t idx_nr = 7;
    constexpr uint32_t idx_npr = 8;
    constexpr uint32_t idx_pq_rs = 9;
    initial_goals.push(saved_expr_pool_.make("bool", {saved_expr_pool_.make(idx_p)}));
    initial_goals.push(saved_expr_pool_.make("bool", {saved_expr_pool_.make(idx_q)}));
    initial_goals.push(saved_expr_pool_.make("bool", {saved_expr_pool_.make(idx_r)}));
    initial_goals.push(saved_expr_pool_.make("bool", {saved_expr_pool_.make(idx_s)}));
    initial_goals.push(saved_expr_pool_.make("or", {
        saved_expr_pool_.make(idx_p),
        saved_expr_pool_.make(idx_q),
        saved_expr_pool_.make(idx_pq),
    }));
    initial_goals.push(saved_expr_pool_.make("or", {
        saved_expr_pool_.make(idx_r),
        saved_expr_pool_.make(idx_s),
        saved_expr_pool_.make(idx_rs),
    }));
    initial_goals.push(saved_expr_pool_.make("not", {
        saved_expr_pool_.make(idx_p),
        saved_expr_pool_.make(idx_np),
    }));
    initial_goals.push(saved_expr_pool_.make("not", {
        saved_expr_pool_.make(idx_r),
        saved_expr_pool_.make(idx_nr),
    }));
    initial_goals.push(saved_expr_pool_.make("or", {
        saved_expr_pool_.make(idx_np),
        saved_expr_pool_.make(idx_nr),
        saved_expr_pool_.make(idx_npr),
    }));
    initial_goals.push(saved_expr_pool_.make("and", {
        saved_expr_pool_.make(idx_pq),
        saved_expr_pool_.make(idx_rs),
        saved_expr_pool_.make(idx_pq_rs),
    }));
    initial_goals.push(saved_expr_pool_.make("and", {
        saved_expr_pool_.make(idx_pq_rs),
        saved_expr_pool_.make(idx_npr),
        saved_expr_pool_.make("true", {}),
    }));

    basic_solver_session session(database, initial_goals, kSatBudget, kSeed);
    std::set<solution> expected = {
        {true_atom, true_atom, false_atom, true_atom},
        {true_atom, false_atom, false_atom, true_atom},
        {false_atom, true_atom, true_atom, true_atom},
        {false_atom, true_atom, true_atom, false_atom},
        {false_atom, true_atom, false_atom, true_atom},
    };
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_p))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_q))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_r))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_s)))};
        });
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
    constexpr uint32_t idx_x = 0;
    constexpr uint32_t idx_y = 1;
    constexpr uint32_t idx_s = 2;
    const expr* ten = saved_expr_pool_.make("zero", {});
    for (int i = 0; i < 10; ++i)
        ten = saved_expr_pool_.make("suc", {ten});
    initial_goals.push(saved_expr_pool_.make("add", {
        saved_expr_pool_.make(idx_x),
        saved_expr_pool_.make(idx_y),
        saved_expr_pool_.make(idx_s),
    }));
    initial_goals.push(saved_expr_pool_.make("lt", {
        saved_expr_pool_.make(idx_s),
        ten,
    }));

    basic_solver_session session(database, initial_goals, kPeanoBudget, kSeed);
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_x))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_y)))};
        });
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
    constexpr uint32_t idx_x = 0;
    constexpr uint32_t idx_y = 1;
    const expr* ten = saved_expr_pool_.make("zero", {});
    for (int i = 0; i < 10; ++i)
        ten = saved_expr_pool_.make("suc", {ten});
    initial_goals.push(saved_expr_pool_.make("nat", {saved_expr_pool_.make(idx_x)}));
    initial_goals.push(saved_expr_pool_.make("nat", {saved_expr_pool_.make(idx_y)}));
    initial_goals.push(saved_expr_pool_.make("add", {
        saved_expr_pool_.make(idx_x),
        saved_expr_pool_.make(idx_y),
        ten,
    }));

    basic_solver_session session(database, initial_goals, kPeanoBudget, kSeed);
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_x))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_y)))};
        });
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
    constexpr uint32_t idx_x = 0;
    constexpr uint32_t idx_y = 1;
    const expr* eight = saved_expr_pool_.make("zero", {});
    for (int i = 0; i < 8; ++i)
        eight = saved_expr_pool_.make("suc", {eight});
    initial_goals.push(saved_expr_pool_.make("nat", {saved_expr_pool_.make(idx_x)}));
    initial_goals.push(saved_expr_pool_.make("nat", {saved_expr_pool_.make(idx_y)}));
    initial_goals.push(saved_expr_pool_.make("mul", {
        saved_expr_pool_.make(idx_x),
        saved_expr_pool_.make(idx_y),
        eight,
    }));

    basic_solver_session session(database, initial_goals, kPeanoBudget, kSeed);
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_x))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_y)))};
        });
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
    constexpr uint32_t idx_x = 0;
    constexpr uint32_t idx_y = 1;
    constexpr uint32_t idx_z = 2;
    constexpr uint32_t idx_s = 3;
    constexpr uint32_t idx_t = 4;
    const expr* bound = saved_expr_pool_.make("zero", {});
    for (int i = 0; i < 4; ++i)
        bound = saved_expr_pool_.make("suc", {bound});
    initial_goals.push(saved_expr_pool_.make("nat", {saved_expr_pool_.make(idx_x)}));
    initial_goals.push(saved_expr_pool_.make("nat", {saved_expr_pool_.make(idx_y)}));
    initial_goals.push(saved_expr_pool_.make("nat", {saved_expr_pool_.make(idx_z)}));
    initial_goals.push(saved_expr_pool_.make("add", {
        saved_expr_pool_.make(idx_x),
        saved_expr_pool_.make(idx_y),
        saved_expr_pool_.make(idx_s),
    }));
    initial_goals.push(saved_expr_pool_.make("add", {
        saved_expr_pool_.make(idx_x),
        saved_expr_pool_.make(idx_z),
        saved_expr_pool_.make(idx_t),
    }));
    initial_goals.push(saved_expr_pool_.make("lt", {
        saved_expr_pool_.make(idx_s),
        bound,
    }));
    initial_goals.push(saved_expr_pool_.make("lt", {
        saved_expr_pool_.make(idx_t),
        bound,
    }));

    basic_solver_session session(database, initial_goals, kPeanoBudget, kSeed);
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_x))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_y))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_z)))};
        });
}

TEST_F(BasicSolverSessionTest, EnumeratesCatalanTreesWithFiveNodes) {
    static constexpr size_t kCatalanBudget = 70;

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
    database.push(rule{
        saved_expr_pool_.make("wf", {saved_expr_pool_.make("bin", {rv6, rv7})}),
        {saved_expr_pool_.make("wf", {rv6}), saved_expr_pool_.make("wf", {rv7})}});

    database.push(rule{saved_expr_pool_.make("nodes", {nil, zero}), {}});

    const expr* one = saved_expr_pool_.make("suc", {zero});
    const expr* rv8 = saved_expr_pool_.make(0);
    const expr* rv9 = saved_expr_pool_.make(1);
    const expr* rv10 = saved_expr_pool_.make(2);
    const expr* rv11 = saved_expr_pool_.make(3);
    const expr* rv12 = saved_expr_pool_.make(4);
    const expr* rv13 = saved_expr_pool_.make(5);
    database.push(rule{
        saved_expr_pool_.make("nodes", {saved_expr_pool_.make("bin", {rv8, rv9}), rv10}),
        {saved_expr_pool_.make("nodes", {rv8, rv11}),
            saved_expr_pool_.make("nodes", {rv9, rv12}),
            saved_expr_pool_.make("add", {rv11, rv12, rv13}),
            saved_expr_pool_.make("add", {one, rv13, rv10})}});

    constexpr uint32_t idx_t = 0;
    const expr* five = saved_expr_pool_.make("zero", {});
    for (int i = 0; i < 5; ++i)
        five = saved_expr_pool_.make("suc", {five});
    initial_goals.push(saved_expr_pool_.make("wf", {saved_expr_pool_.make(idx_t)}));
    initial_goals.push(saved_expr_pool_.make("nodes", {saved_expr_pool_.make(idx_t), five}));

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
    }
    EXPECT_EQ(visited.size(), 42u);
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
    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    constexpr uint32_t idx_c = 2;
    initial_goals.push(saved_expr_pool_.make("f", {}));
    initial_goals.push(saved_expr_pool_.make("g", {
        saved_expr_pool_.make(idx_a),
        saved_expr_pool_.make(idx_b),
        saved_expr_pool_.make(idx_c),
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
    database.push(rule{saved_expr_pool_.make("reach", {a, b}), {}});
    database.push(rule{saved_expr_pool_.make("reach", {a, c}), {}});
    database.push(rule{saved_expr_pool_.make("reach", {a, d}), {}});

    constexpr uint32_t idx_y = 0;
    initial_goals.push(saved_expr_pool_.make("reach", {a, saved_expr_pool_.make(idx_y)}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<solution> expected{{b}, {c}, {d}};
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_y)))};
        });
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
    constexpr uint32_t idx_n = 0;
    const expr* eight = saved_expr_pool_.make("zero", {});
    for (int i = 0; i < 8; ++i)
        eight = saved_expr_pool_.make("suc", {eight});
    initial_goals.push(saved_expr_pool_.make("even", {saved_expr_pool_.make(idx_n)}));
    initial_goals.push(saved_expr_pool_.make("lt", {
        saved_expr_pool_.make(idx_n),
        eight,
    }));

    basic_solver_session session(database, initial_goals, kPeanoBudget, kSeed);
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_n)))};
        });
}

TEST_F(BasicSolverSessionTest, EnumeratesListSplitsForThreeElementList) {
    const expr* nil = saved_expr_pool_.make("nil", {});
    const expr* a = saved_expr_pool_.make("a", {});
    const expr* b = saved_expr_pool_.make("b", {});
    const expr* list_ab = saved_expr_pool_.make("cons", {a, saved_expr_pool_.make("cons", {b, nil})});
    const expr* list_a = saved_expr_pool_.make("cons", {a, nil});
    const expr* list_b = saved_expr_pool_.make("cons", {b, nil});

    database.push(rule{saved_expr_pool_.make("append", {nil, list_ab, list_ab}), {}});
    database.push(rule{saved_expr_pool_.make("append", {list_a, list_b, list_ab}), {}});

    constexpr uint32_t idx_l1 = 0;
    constexpr uint32_t idx_l2 = 1;
    initial_goals.push(saved_expr_pool_.make("append", {
        saved_expr_pool_.make(idx_l1),
        saved_expr_pool_.make(idx_l2),
        list_ab,
    }));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    std::set<solution> expected{{nil, list_ab}, {list_a, list_b}};
    enumerate_all_solutions(
        session,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_l1))), saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_l2)))};
        });
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
