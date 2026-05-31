// Integration-style tests for basic_solver_session — real wiring, no mocks.

#include <functional>
#include <set>
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
#include "value_objects/sim_termination.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

using solution = std::vector<const expr*>;

namespace {

locator& bind_saved_loc(trail& t, locator& l) {
    l.bind_as<i_log_to_current_trail_frame>(t);
    return l;
}

void next_until_refuted(
    basic_solver_session& session,
    std::set<solution> expected,
    const std::function<solution()>& get_solution) {
    std::set<solution> visited;
    while (true) {
        if (!session.next())
            break;
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

struct BasicSolverSessionTest : public ::testing::Test {
    static constexpr size_t kMaxResolutions = 32;
    static constexpr uint32_t kSeed = 42;

    db database;
    initial_goal_exprs initial_goals;

    trail saved_trail_;
    locator saved_loc_;
    expr_pool saved_expr_pool_{bind_saved_loc(saved_trail_, saved_loc_)};
};

// Tier A — session API smoke

TEST_F(BasicSolverSessionTest, ConstructsWithEmptyDbAndGoals) {
    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next());
    EXPECT_TRUE(session.solved());
    EXPECT_FALSE(session.next());
}

TEST_F(BasicSolverSessionTest, VacuousSolvedTick) {
    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next());
    EXPECT_TRUE(session.solved());
    EXPECT_TRUE(session.derive_decision_lemma().get_resolutions().empty());
    EXPECT_FALSE(session.next());
}

TEST_F(BasicSolverSessionTest, NextFalseMeansRefuted) {
    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    while (session.next()) {}
    EXPECT_FALSE(session.next());
}

TEST_F(BasicSolverSessionTest, SolvedOnlyWhenPausedAtYield) {
    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    EXPECT_FALSE(session.solved());
    ASSERT_TRUE(session.next());
    EXPECT_TRUE(session.solved());
    ASSERT_FALSE(session.next());
    EXPECT_FALSE(session.solved());
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
    ASSERT_TRUE(session.next());
    ASSERT_TRUE(session.solved());
    EXPECT_EQ(*saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a))),
        *abc);
    EXPECT_EQ(*saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_b))),
        *_123);

    EXPECT_FALSE(session.next());
    EXPECT_TRUE(std::holds_alternative<expr::var>(
        session.normalize(saved_expr_pool_.make(idx_a))->content));
}

TEST_F(BasicSolverSessionTest, ImportSurvivesNextTick) {
    const expr* abc = saved_expr_pool_.make("abc", {});
    database.push(rule{saved_expr_pool_.make("f", {abc}), {}});

    constexpr uint32_t idx_a = 0;
    initial_goals.push(saved_expr_pool_.make("f", {saved_expr_pool_.make(idx_a)}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next());
    ASSERT_TRUE(session.solved());
    const expr* kept = saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a)));
    EXPECT_EQ(*kept, *abc);

    EXPECT_FALSE(session.next());
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
    ASSERT_TRUE(session.next());
    ASSERT_TRUE(session.solved());
    EXPECT_FALSE(session.derive_decision_lemma().get_resolutions().empty());
}

TEST_F(BasicSolverSessionTest, DeriveResolutionLemmaOnDemand) {
    const expr* goal = saved_expr_pool_.make("f", {});
    const expr* head = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
    database.push(rule{head, {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next());
    ASSERT_TRUE(session.solved());
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
    ASSERT_TRUE(session.next());
    ASSERT_TRUE(session.solved());
    const lemma decision1 = session.derive_decision_lemma();
    const lemma resolution1 = session.derive_resolution_lemma();
    EXPECT_EQ(decision1.get_resolutions().size(), 1u);

    ASSERT_TRUE(session.next());
    ASSERT_TRUE(session.solved());
    const lemma decision2 = session.derive_decision_lemma();
    const lemma resolution2 = session.derive_resolution_lemma();
    EXPECT_TRUE(decision2.get_resolutions().empty());
    EXPECT_FALSE(decision1.get_resolutions() == decision2.get_resolutions()
        && resolution1.get_resolutions() == resolution2.get_resolutions());
}

// Tier B — solver outcomes

TEST_F(BasicSolverSessionTest, FindsSingleUnitSolution) {
    const expr* goal = saved_expr_pool_.make("f", {});
    initial_goals.push(goal);
    database.push(rule{saved_expr_pool_.make("f", {}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next());
    EXPECT_TRUE(session.solved());
    EXPECT_TRUE(session.derive_decision_lemma().get_resolutions().empty());
    EXPECT_FALSE(session.next());
}

TEST_F(BasicSolverSessionTest, RefutesWhenNoCandidates) {
    initial_goals.push(saved_expr_pool_.make("f", {}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next());
    EXPECT_FALSE(session.solved());
    EXPECT_FALSE(session.next());
}

TEST_F(BasicSolverSessionTest, FindsClauseDerivedUnitSolution) {
    const expr* g_body = saved_expr_pool_.make("g", {});
    initial_goals.push(saved_expr_pool_.make("f", {}));
    database.push(rule{saved_expr_pool_.make("f", {}), {g_body}});
    database.push(rule{saved_expr_pool_.make("g", {}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next());
    EXPECT_TRUE(session.solved());
    EXPECT_TRUE(session.derive_decision_lemma().get_resolutions().empty());
    EXPECT_FALSE(session.next());
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
    ASSERT_TRUE(session.next());
    EXPECT_TRUE(session.solved());
    EXPECT_EQ(*saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a))),
        *abc);
    EXPECT_EQ(*saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_b))),
        *_123);
    EXPECT_FALSE(session.next());
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
    ASSERT_TRUE(session.next());
    EXPECT_TRUE(session.solved());
    EXPECT_EQ(*saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_a))),
        *abc);
    EXPECT_EQ(*saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_b))),
        *_123);
    EXPECT_FALSE(session.next());
}

TEST_F(BasicSolverSessionTest, RefutesAfterCdclOnUnsatClauseBranches) {
    const expr* b = saved_expr_pool_.make("b", {});
    const expr* c = saved_expr_pool_.make("c", {});
    database.push(rule{saved_expr_pool_.make("a", {}), {b}});
    database.push(rule{saved_expr_pool_.make("a", {}), {c}});
    initial_goals.push(saved_expr_pool_.make("a", {}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    size_t yields = 0;
    bool saw_conflicted = false;
    while (session.next()) {
        ++yields;
        if (session.solved())
            FAIL() << "unexpected solved termination";
        saw_conflicted = true;
    }
    ASSERT_GE(yields, 2u);
    EXPECT_TRUE(saw_conflicted);
}

// Tier C — enumeration

TEST_F(BasicSolverSessionTest, EnumeratesTwoGroundChoiceSolutions) {
    initial_goals.push(saved_expr_pool_.make("f", {}));
    database.push(rule{saved_expr_pool_.make("f", {}), {}});
    database.push(rule{saved_expr_pool_.make("f", {}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    size_t solved_count = 0;
    while (session.next()) {
        if (session.solved())
            ++solved_count;
    }
    EXPECT_EQ(solved_count, 2u);
}

TEST_F(BasicSolverSessionTest, RefutesAfterEnumeratingAllGroundBranches) {
    initial_goals.push(saved_expr_pool_.make("f", {}));
    database.push(rule{saved_expr_pool_.make("f", {}), {}});
    database.push(rule{saved_expr_pool_.make("f", {}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    size_t solved_count = 0;
    while (session.next()) {
        if (session.solved())
            ++solved_count;
    }
    EXPECT_EQ(solved_count, 2u);
}

TEST_F(BasicSolverSessionTest, EnumeratesTwoVarChoiceSolutions) {
    const expr* abc = saved_expr_pool_.make("abc", {});
    const expr* xyz = saved_expr_pool_.make("xyz", {});
    database.push(rule{saved_expr_pool_.make("f", {abc}), {}});
    database.push(rule{saved_expr_pool_.make("f", {xyz}), {}});

    constexpr uint32_t idx_a = 0;
    initial_goals.push(saved_expr_pool_.make("f", {saved_expr_pool_.make(idx_a)}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    next_until_refuted(
        session,
        {{abc}, {xyz}},
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
    next_until_refuted(
        session,
        {{abc}, {xyz}},
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
    next_until_refuted(
        session,
        {{abc}, {xyz}},
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
    next_until_refuted(
        session,
        {{a}, {b}, {c}, {d}},
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
    size_t solved_count = 0;
    while (session.next()) {
        if (session.solved())
            ++solved_count;
    }
    EXPECT_EQ(solved_count, 4u);
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
    next_until_refuted(
        session,
        {{two}},
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize(saved_expr_pool_.make(idx_x)))};
        });
}

// Tier F

TEST_F(BasicSolverSessionTest, ConflictedTickNotSolved) {
    initial_goals.push(saved_expr_pool_.make("f", {}));

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next());
    EXPECT_FALSE(session.solved());
    EXPECT_FALSE(session.next());
}

TEST_F(BasicSolverSessionTest, SkippedLemmaCallsOnUnitTicks) {
    initial_goals.push(saved_expr_pool_.make("f", {}));
    database.push(rule{saved_expr_pool_.make("f", {}), {}});

    basic_solver_session session(database, initial_goals, kMaxResolutions, kSeed);
    ASSERT_TRUE(session.next());
    EXPECT_TRUE(session.solved());
    EXPECT_FALSE(session.next());
}
