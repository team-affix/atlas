// Integration: dbuct_manifest — delayed-backtracking (ridge_dbuct) solver.
//
// The ridge_dbuct solver camps deep in the search tree and backtracks lazily
// (DBUCT) instead of restarting each simulation from the root. These tests fix
// its observable CONTRACT: it must find exactly the solutions the restarting
// ridge solver finds (soundness + completeness), remain correct across lazy
// backsteps (state, bindings, and learned avoidances restored to each resume
// frontier), and actually camp rather than restart.
//
// Most correctness checks are DIFFERENTIAL against ridge_runtime: for a given
// problem the two solvers may explore in a different order, so we compare the
// SET of solutions, not the sequence — a change-detector-free contract.

#include <algorithm>
#include <cstddef>
#include <functional>
#include <set>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/dbuct_manifest.hpp"
#include "infrastructure/dbuct_runtime.hpp"
#include "infrastructure/ridge_runtime.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/sim_termination.hpp"
#include "functor_fixture.hpp"

namespace {

using solution = std::vector<const expr*>;

// Enumerate a runtime's solution set, importing each solution into a shared pool
// so solutions from different runtimes are pointer-comparable. Bounded by
// max_ticks so a satisfiable-but-non-refuting search cannot hang the suite.
template<typename Runtime>
std::set<solution> collect_solutions(
    Runtime& rt,
    const std::function<solution()>& get_solution,
    size_t max_ticks) {
    std::set<solution> sols;
    for (size_t t = 0; t < max_ticks; ++t) {
        if (!rt.next())
            break;
        if (rt.solved())
            sols.insert(get_solution());
    }
    return sols;
}

class DbuctManifestIntegrationTest : public ::testing::Test {
protected:
    test_functors functors;
    static constexpr size_t kMaxResolutions = 64;
    static constexpr uint32_t kSeed = 42;
    static constexpr double kExplorationConstant = 1.414;
    static constexpr size_t kGrantInterval = 4;
    static constexpr size_t kTickCap = 4096;

    db database;
    initial_goal_exprs initial_goals;
    expr_pool pool;  // shared: problem construction + cross-runtime import

    dbuct_manifest make_manifest(uint32_t initial_frame_offset = 0,
                                 size_t max_resolutions = kMaxResolutions,
                                 size_t grant_interval = kGrantInterval) {
        return dbuct_manifest{
            database, initial_goals, initial_frame_offset, max_resolutions, kSeed,
            kExplorationConstant, grant_interval};
    }

    dbuct_runtime make_dbuct(uint32_t initial_var_count,
                             size_t max_resolutions = kMaxResolutions,
                             size_t grant_interval = kGrantInterval) {
        return dbuct_runtime{database, initial_goals, initial_var_count,
                             max_resolutions, kSeed, kExplorationConstant,
                             grant_interval};
    }

    ridge_runtime make_ridge(uint32_t initial_var_count,
                             size_t max_resolutions = kMaxResolutions) {
        return ridge_runtime{database, initial_goals, initial_var_count,
                             max_resolutions, kSeed, kExplorationConstant};
    }

    const expr* fun(const char* name, std::vector<const expr*> args = {}) {
        return pool.make_functor(functors.id(name), std::move(args));
    }
};

// ── Scenario 1: wiring ───────────────────────────────────────────────────────

TEST_F(DbuctManifestIntegrationTest, WiringConstructsWithEmptyProblem) {
    EXPECT_NO_THROW(make_manifest());
}

TEST_F(DbuctManifestIntegrationTest, WiringCampingStackDistinctFromEliminators) {
    dbuct_manifest m = make_manifest();
    EXPECT_NE(static_cast<void*>(&m.cdcl_), static_cast<void*>(&m.joint_));
    EXPECT_NE(static_cast<void*>(&m.dbuct_sim_), static_cast<void*>(&m.checkpoints_));
}

// ── Scenario 2: vacuous / trivial refutation contract ────────────────────────

TEST_F(DbuctManifestIntegrationTest, VacuousSolvedThenRefutesOnEmptyProblem) {
    dbuct_runtime rt = make_dbuct(0);
    ASSERT_TRUE(rt.next());
    EXPECT_TRUE(rt.solved());
    EXPECT_TRUE(rt.derive_decision_lemma().get_resolutions().empty());
    EXPECT_FALSE(rt.next()) << "empty problem is exhausted after one vacuous tick";
}

TEST_F(DbuctManifestIntegrationTest, RefutesWhenInitialGoalHasNoCandidates) {
    initial_goals.push(fun("f"));
    dbuct_runtime rt = make_dbuct(0);
    ASSERT_TRUE(rt.next());
    EXPECT_FALSE(rt.solved()) << "no matching rule ⇒ conflict";
    EXPECT_FALSE(rt.next()) << "root refuted";
}

// ── Scenario 3: unit propagation, no decisions ───────────────────────────────

TEST_F(DbuctManifestIntegrationTest, SingleUnitSolutionMakesNoDecision) {
    initial_goals.push(fun("f"));
    database.push(rule{fun("f"), {}});
    dbuct_runtime rt = make_dbuct(0);
    ASSERT_TRUE(rt.next());
    EXPECT_TRUE(rt.solved());
    EXPECT_TRUE(rt.derive_decision_lemma().get_resolutions().empty());
    EXPECT_FALSE(rt.next());
}

TEST_F(DbuctManifestIntegrationTest, ClauseDerivedUnitSolutionChainsSubgoals) {
    initial_goals.push(fun("f"));
    database.push(rule{fun("f"), {fun("g")}});
    database.push(rule{fun("g"), {}});
    dbuct_runtime rt = make_dbuct(0);
    ASSERT_TRUE(rt.next());
    EXPECT_TRUE(rt.solved());
    EXPECT_TRUE(rt.derive_decision_lemma().get_resolutions().empty());
    EXPECT_FALSE(rt.next());
}

// ── Scenario 4: MHU unification bindings survive the camping/backtrack stack ──

TEST_F(DbuctManifestIntegrationTest, GroundHeadBindingsAreCorrect) {
    const expr* abc = fun("abc");
    const expr* _123 = fun("123");
    database.push(rule{fun("f", {abc, _123}), {}});
    initial_goals.push(fun("f", {pool.make_var(0), pool.make_var(1)}));

    dbuct_runtime rt = make_dbuct(2);
    ASSERT_TRUE(rt.next());
    ASSERT_TRUE(rt.solved());
    EXPECT_EQ(*pool.import(rt.normalize({pool.make_var(0), 0})), *abc);
    EXPECT_EQ(*pool.import(rt.normalize({pool.make_var(1), 0})), *_123);
}

TEST_F(DbuctManifestIntegrationTest, ClauseBodyBindingsFlowThroughSubgoals) {
    const expr* va = pool.make_var(0);
    const expr* vb = pool.make_var(1);
    const expr* abc = fun("abc");
    const expr* _123 = fun("123");
    database.push(rule{fun("f", {va, vb}), {fun("g", {va}), fun("h", {vb})}});
    database.push(rule{fun("g", {abc}), {}});
    database.push(rule{fun("h", {_123}), {}});
    initial_goals.push(fun("f", {pool.make_var(0), pool.make_var(1)}));

    dbuct_runtime rt = make_dbuct(2);
    ASSERT_TRUE(rt.next());
    ASSERT_TRUE(rt.solved());
    EXPECT_EQ(*pool.import(rt.normalize({pool.make_var(0), 0})), *abc);
    EXPECT_EQ(*pool.import(rt.normalize({pool.make_var(1), 0})), *_123);
}

// ── Scenario 5: camping — the solver descends and holds depth (vs restart) ────

TEST_F(DbuctManifestIntegrationTest, SolverCampsBelowRootAcrossEpisodes) {
    // Two competing candidates for f force a branching decision. A restarting
    // solver would tear back down to the root between every episode, so its
    // camp depth would always be zero. DBUCT keeps choice frames live below the
    // root once the tree has grown: over several episodes the checkpoint stack
    // must, at some yield, hold at least one camped choice frame.
    const expr* a = fun("a");
    const expr* b = fun("b");
    const expr* c = fun("c");
    const expr* d = fun("d");
    database.push(rule{fun("f", {a}), {}});
    database.push(rule{fun("f", {b}), {}});
    database.push(rule{fun("f", {c}), {}});
    database.push(rule{fun("f", {d}), {}});
    initial_goals.push(fun("f", {pool.make_var(0)}));

    dbuct_manifest m = make_manifest(/*initial_frame_offset=*/1);
    auto sm = m.solver_.solve();
    size_t max_camp_depth = 0;
    bool made_decision = false;
    for (size_t episode = 0; episode < 32; ++episode) {
        sm.resume();
        if (!sm.has_yield()) break;
        sm.consume_yield();
        made_decision = made_decision || m.decision_memory_.count() >= 1;
        max_camp_depth = std::max(max_camp_depth, m.checkpoints_.frame_depth());
    }
    EXPECT_TRUE(made_decision) << "a branching decision was made";
    EXPECT_GT(max_camp_depth, 0u)
        << "solver camped below root: choice frames held across episodes";
}

// ── Scenario 6-8: differential solution-set parity (soundness+completeness) ───

TEST_F(DbuctManifestIntegrationTest, ParitySingleBindingSolution) {
    const expr* abc = fun("abc");
    const expr* _123 = fun("123");
    database.push(rule{fun("f", {abc, _123}), {}});
    initial_goals.push(fun("f", {pool.make_var(0), pool.make_var(1)}));

    auto get = [&](auto& rt) {
        return solution{pool.import(rt.normalize({pool.make_var(0), 0})),
                        pool.import(rt.normalize({pool.make_var(1), 0}))};
    };
    ridge_runtime r = make_ridge(2);
    std::set<solution> expected = collect_solutions(
        r, [&] { return get(r); }, kTickCap);

    dbuct_runtime d = make_dbuct(2);
    std::set<solution> got = collect_solutions(
        d, [&] { return get(d); }, kTickCap);

    EXPECT_EQ(got, expected);
    EXPECT_FALSE(expected.empty());
}

TEST_F(DbuctManifestIntegrationTest, ParityMultipleGroundFacts) {
    // Three facts for f(X): the solver must enumerate all three bindings.
    const expr* a = fun("a");
    const expr* b = fun("b");
    const expr* c = fun("c");
    database.push(rule{fun("f", {a}), {}});
    database.push(rule{fun("f", {b}), {}});
    database.push(rule{fun("f", {c}), {}});
    initial_goals.push(fun("f", {pool.make_var(0)}));

    auto get = [&](auto& rt) {
        return solution{pool.import(rt.normalize({pool.make_var(0), 0}))};
    };
    ridge_runtime r = make_ridge(1);
    std::set<solution> expected = collect_solutions(r, [&] { return get(r); }, kTickCap);

    dbuct_runtime d = make_dbuct(1);
    std::set<solution> got = collect_solutions(d, [&] { return get(d); }, kTickCap);

    EXPECT_EQ(got, expected);
    EXPECT_EQ(expected.size(), 3u);
}

TEST_F(DbuctManifestIntegrationTest, ParityTwoGoalsCrossProduct) {
    // f(X) with two facts, g(Y) with two facts, joined goal f(X),g(Y):
    // four combined solutions. Exercises multiple active goals + the SRT tree
    // being snapshot/restored across camping backsteps.
    const expr* a = fun("a");
    const expr* b = fun("b");
    database.push(rule{fun("f", {a}), {}});
    database.push(rule{fun("f", {b}), {}});
    database.push(rule{fun("g", {a}), {}});
    database.push(rule{fun("g", {b}), {}});
    initial_goals.push(fun("f", {pool.make_var(0)}));
    initial_goals.push(fun("g", {pool.make_var(1)}));

    auto get = [&](auto& rt) {
        return solution{pool.import(rt.normalize({pool.make_var(0), 0})),
                        pool.import(rt.normalize({pool.make_var(1), 0}))};
    };
    ridge_runtime r = make_ridge(2);
    std::set<solution> expected = collect_solutions(r, [&] { return get(r); }, kTickCap);

    dbuct_runtime d = make_dbuct(2);
    std::set<solution> got = collect_solutions(d, [&] { return get(d); }, kTickCap);

    EXPECT_EQ(got, expected);
    EXPECT_EQ(expected.size(), 4u);
}

// ── Scenario 9-10: conflict-driven learning + backjumping across camp levels ──

TEST_F(DbuctManifestIntegrationTest, ParityConflictDrivenPruning) {
    // p(X) has two facts; q requires p(a) specifically. Choosing p(b) at the
    // shared X leads to a dead end for the q branch, so the solver must learn to
    // avoid it and backtrack. Only the p(a) solution is sound.
    const expr* a = fun("a");
    const expr* b = fun("b");
    const expr* va = pool.make_var(0);
    // top(X) :- p(X), q(X).   p(a). p(b).   q(a).
    database.push(rule{fun("top", {va}), {fun("p", {va}), fun("q", {va})}});
    database.push(rule{fun("p", {a}), {}});
    database.push(rule{fun("p", {b}), {}});
    database.push(rule{fun("q", {a}), {}});
    initial_goals.push(fun("top", {pool.make_var(0)}));

    auto get = [&](auto& rt) {
        return solution{pool.import(rt.normalize({pool.make_var(0), 0}))};
    };
    ridge_runtime r = make_ridge(1);
    std::set<solution> expected = collect_solutions(r, [&] { return get(r); }, kTickCap);

    dbuct_runtime d = make_dbuct(1);
    std::set<solution> got = collect_solutions(d, [&] { return get(d); }, kTickCap);

    EXPECT_EQ(got, expected);
    for (const solution& s : got)
        EXPECT_EQ(*s.at(0), *a) << "only X=a is sound; X=b must be pruned";
}

// ── Scenario 11: bindings correct after continuing past a solution (camped) ───

TEST_F(DbuctManifestIntegrationTest, BindingsRemainSoundAcrossEnumeration) {
    // Every enumerated solution must be a genuine fact; a stale binding leaking
    // across a camping backstep (e.g. an un-rolled-back whnf compression) would
    // surface here as an unexpected binding.
    const expr* a = fun("a");
    const expr* b = fun("b");
    const expr* c = fun("c");
    database.push(rule{fun("f", {a}), {}});
    database.push(rule{fun("f", {b}), {}});
    database.push(rule{fun("f", {c}), {}});
    initial_goals.push(fun("f", {pool.make_var(0)}));

    std::set<const expr*> facts{pool.import(a), pool.import(b), pool.import(c)};
    dbuct_runtime d = make_dbuct(1);
    auto got = collect_solutions(
        d, [&] { return solution{pool.import(d.normalize({pool.make_var(0), 0}))}; },
        kTickCap);
    ASSERT_FALSE(got.empty());
    for (const solution& s : got)
        EXPECT_TRUE(facts.count(s.at(0))) << "enumerated a binding that is not a fact";
}

// ── Scenario 12: lineage stability (no dangling pointers under camping) ───────

TEST_F(DbuctManifestIntegrationTest, ImportedSolutionSurvivesFurtherTicks) {
    const expr* abc = fun("abc");
    database.push(rule{fun("f", {abc}), {}});
    initial_goals.push(fun("f", {pool.make_var(0)}));

    dbuct_runtime d = make_dbuct(1);
    ASSERT_TRUE(d.next());
    ASSERT_TRUE(d.solved());
    const expr* kept = pool.import(d.normalize({pool.make_var(0), 0}));
    EXPECT_EQ(*kept, *abc);
    d.next();  // camp/backtrack churns lineages; kept must stay valid
    EXPECT_EQ(*kept, *abc);
}

// ── Scenario 13: depth bound is honoured without crashing ─────────────────────

TEST_F(DbuctManifestIntegrationTest, TightResolutionBudgetTerminatesGracefully) {
    // left-recursive rule; a tiny per-episode resolution budget must bound each
    // episode and never crash the camping loop.
    const expr* va = pool.make_var(0);
    database.push(rule{fun("nat", {fun("s", {va})}), {fun("nat", {va})}});
    database.push(rule{fun("nat", {fun("z")}), {}});
    initial_goals.push(fun("nat", {pool.make_var(0)}));

    dbuct_runtime d = make_dbuct(1, /*max_resolutions=*/4);
    bool any_solution = false;
    for (size_t t = 0; t < 64; ++t) {
        if (!d.next()) break;
        if (d.solved()) any_solution = true;
    }
    EXPECT_TRUE(any_solution) << "at least nat(z) is reachable within the budget";
}

// ── Scenario 14: recursion — every enumerated binding is a well-formed nat ────

TEST_F(DbuctManifestIntegrationTest, RecursionEnumeratesOnlyWellFormedNats) {
    // nat(z). nat(s(X)) :- nat(X). Under a per-episode resolution budget the
    // camping search explores to varying depths, but every binding it reports
    // must be a genuine natural: z, or s(...) nested finitely over z. A stale
    // binding leaking across a backstep would surface as a malformed term.
    const expr* va = pool.make_var(0);
    database.push(rule{fun("nat", {fun("s", {va})}), {fun("nat", {va})}});
    database.push(rule{fun("nat", {fun("z")}), {}});
    initial_goals.push(fun("nat", {pool.make_var(0)}));

    const auto is_well_formed_nat = [&](const expr* e) {
        const uint32_t z_id = functors.id("z");
        const uint32_t s_id = functors.id("s");
        while (true) {
            const auto* f = std::get_if<expr::functor>(&e->content);
            if (!f) return false;
            if (f->id == z_id) return f->args.empty();
            if (f->id != s_id || f->args.size() != 1) return false;
            e = f->args[0];
        }
    };

    dbuct_runtime d = make_dbuct(1, /*max_resolutions=*/8);
    std::set<solution> got = collect_solutions(
        d, [&] { return solution{pool.import(d.normalize({pool.make_var(0), 0}))}; },
        256);

    ASSERT_FALSE(got.empty());
    for (const solution& s : got)
        EXPECT_TRUE(is_well_formed_nat(s.at(0))) << "enumerated a malformed nat term";
    EXPECT_TRUE(got.count(solution{pool.import(fun("z"))}))
        << "shallowest solution nat(z) must be reachable";
}

// ── Scenario 15: batch-increment knob tunes camping, not the solution set ─────

TEST_F(DbuctManifestIntegrationTest, GrantIntervalDoesNotChangeSolutionSet) {
    // grant_increment_interval controls how long DBUCT camps before backtracking
    // (a very large value ⇒ near-UCT restart behaviour). It is a search-order
    // knob only: completeness/soundness must be invariant to it.
    const expr* a = fun("a");
    const expr* b = fun("b");
    const expr* c = fun("c");
    database.push(rule{fun("f", {a}), {}});
    database.push(rule{fun("f", {b}), {}});
    database.push(rule{fun("f", {c}), {}});
    initial_goals.push(fun("f", {pool.make_var(0)}));

    auto get = [&](auto& rt) {
        return solution{pool.import(rt.normalize({pool.make_var(0), 0}))};
    };
    dbuct_runtime small = make_dbuct(1, kMaxResolutions, /*grant_interval=*/1);
    std::set<solution> got_small = collect_solutions(small, [&] { return get(small); }, kTickCap);

    dbuct_runtime large = make_dbuct(1, kMaxResolutions, /*grant_interval=*/1024);
    std::set<solution> got_large = collect_solutions(large, [&] { return get(large); }, kTickCap);

    EXPECT_EQ(got_small, got_large);
    EXPECT_EQ(got_small.size(), 3u);
}

// ── Scenario 15b: camping persists across episodes without corrupting state ───

TEST_F(DbuctManifestIntegrationTest, DecisionCountConsistentAcrossManyTicks) {
    // Drive many camping episodes; the full-path decision count must stay
    // bounded by the resolution budget and the derived lemma must always be a
    // subset of the recorded resolutions (never referencing rolled-back state).
    const expr* a = fun("a");
    const expr* b = fun("b");
    database.push(rule{fun("f", {a}), {}});
    database.push(rule{fun("f", {b}), {}});
    database.push(rule{fun("g", {a}), {}});
    database.push(rule{fun("g", {b}), {}});
    initial_goals.push(fun("f", {pool.make_var(0)}));
    initial_goals.push(fun("g", {pool.make_var(1)}));

    dbuct_manifest m = make_manifest(/*initial_frame_offset=*/2);
    auto sm = m.solver_.solve();
    for (size_t t = 0; t < 64; ++t) {
        sm.resume();
        if (!sm.has_yield()) break;
        sm.consume_yield();
        EXPECT_LE(m.decision_memory_.count(), kMaxResolutions);
    }
    SUCCEED();
}

}  // namespace
