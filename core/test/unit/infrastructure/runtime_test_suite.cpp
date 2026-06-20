// Shared runtime integration tests — exercised through i_runtime& for each runtime_kind.
//
// Ridge instantiations may fail independently of basic; treat failures as triage input,
// not automatic production fixes beyond i_runtime wiring.
//
// solved() is only valid immediately after next() returned true.
// Harness: enumerate_all_solutions, next_until_refuted — binding enumeration.

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/basic_runtime.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/ridge_runtime.hpp"
#include "infrastructure/horizon_runtime.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/functor_names.hpp"
#include "infrastructure/var_names.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/sim_termination.hpp"
#include "functor_fixture.hpp"

inline constexpr size_t kMaxResolutions = 32;
inline constexpr uint32_t kSeed = 41;
inline constexpr double kRidgeExplorationConstant = 1.414;

enum class runtime_kind { basic, ridge, horizon };

// Type-erased runtime reference for testing across all three runtime types.
struct runtime_ref {
    using variant_t = std::variant<basic_runtime*, ridge_runtime*, horizon_runtime*>;
    explicit runtime_ref(variant_t v) : v_(v) {}
    bool next() { return std::visit([](auto* r) { return r->next(); }, v_); }
    bool solved() const { return std::visit([](const auto* r) { return r->solved(); }, v_); }
    const expr* normalize(framed_expr fe) {
        return std::visit([fe](auto* r) { return r->normalize(fe); }, v_);
    }
    lemma derive_decision_lemma() const {
        return std::visit([](const auto* r) { return r->derive_decision_lemma(); }, v_);
    }
    lemma derive_resolution_lemma() const {
        return std::visit([](const auto* r) { return r->derive_resolution_lemma(); }, v_);
    }
private:
    variant_t v_;
};

struct runtime_session_holder {
    test_functors functors;
    std::optional<basic_runtime> basic;
    std::optional<ridge_runtime> ridge;
    std::optional<horizon_runtime> horizon;
    std::optional<runtime_ref> ref;
};

runtime_ref& make_runtime_session(
    runtime_session_holder& holder,
    runtime_kind kind,
    db& database,
    initial_goal_exprs& goals,
    uint32_t initial_frame_offset,
    size_t max_resolutions,
    uint32_t seed) {
    switch (kind) {
        case runtime_kind::basic:
            holder.basic.emplace(
                database, goals, initial_frame_offset, max_resolutions, seed);
            holder.ref.emplace(runtime_ref::variant_t{&*holder.basic});
            return *holder.ref;
        case runtime_kind::ridge:
            holder.ridge.emplace(
                database,
                goals,
                initial_frame_offset,
                max_resolutions,
                seed,
                kRidgeExplorationConstant);
            holder.ref.emplace(runtime_ref::variant_t{&*holder.ridge});
            return *holder.ref;
        case runtime_kind::horizon:
            holder.horizon.emplace(
                database,
                goals,
                initial_frame_offset,
                max_resolutions,
                seed,
                kRidgeExplorationConstant);
            holder.ref.emplace(runtime_ref::variant_t{&*holder.horizon});
            return *holder.ref;
    }
    throw std::logic_error("unknown runtime_kind");
}

namespace {

using solution = std::vector<const expr*>;

void print_solution(
    size_t solution_index, expr_printer& printer, const solution& s) {
    std::cout << "solution " << solution_index << '\n';
    for (const expr* e : s) {
        std::cout << "  ";
        printer.print(e);
        std::cout << '\n';
    }
}

void enumerate_all_solutions(
    runtime_ref& session,
    expr_printer& printer,
    std::set<solution> expected,
    const std::function<solution()>& get_solution) {
    std::set<solution> visited;
    size_t solution_index = 0;
    while (!expected.empty()) {
        if (!session.next()) {
            std::cout << "solver stopped early; expected_remaining=" << expected.size()
                      << "visited=" << visited.size() << '\n';
            for (const solution& s : expected)
                print_solution(solution_index++, printer, s);
            FAIL() << "solver stopped before all expected solutions found";
        }
        if (!session.solved())
            continue;
        const solution s = get_solution();
        if (visited.count(s))
            continue;
        print_solution(solution_index, printer, s);
        auto it = expected.find(s);
        if (it == expected.end()) {
            std::cout << "unexpected solution (not in expected set)\n";
            FAIL() << "unexpected solution";
        }
        expected.erase(it);
        visited.insert(s);
        ++solution_index;
    }
}

void next_until_refuted(
    runtime_ref& session,
    expr_printer& printer,
    std::set<solution> expected,
    const std::function<solution()>& get_solution) {
    std::set<solution> visited;
    size_t solution_index = 0;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = get_solution();
        if (visited.count(s))
            continue;
        visited.insert(s);
        auto it = expected.find(s);
        if (it == expected.end()) {
            std::cout << "unexpected solution (not in expected set)\n";
            print_solution(solution_index, printer, s);
            FAIL() << "unexpected solution";
        }
        expected.erase(it);
        ++solution_index;
    }
    if (!expected.empty()) {
        std::cout << "solver refuted early; expected_remaining=" << expected.size()
                  << " visited=" << visited.size() << '\n';
        for (const solution& s : visited)
            print_solution(solution_index++, printer, s);
        FAIL() << "solver refuted before all expected solutions found";
    }
}

}  // namespace

struct RuntimeTestBase {
    db database;
    initial_goal_exprs initial_goals;

    var_names saved_var_names_;
    functor_names saved_functor_names_;
    expr_printer saved_printer_{std::cout, saved_var_names_, saved_functor_names_};
    expr_pool saved_expr_pool_;
};

struct RuntimeParamTest
    : RuntimeTestBase
    , ::testing::TestWithParam<runtime_kind> {
    runtime_session_holder holder_;

    runtime_ref& make_session(
        uint32_t initial_frame_offset,
        size_t max_resolutions = kMaxResolutions) {
        return make_runtime_session(
            holder_,
            GetParam(),
            database,
            initial_goals,
            initial_frame_offset,
            max_resolutions,
            kSeed);
    }
};

// Tier A — session API smoke

TEST_P(RuntimeParamTest, ConstructsWithEmptyDbAndGoals) {
    static constexpr size_t kInitialVarCount = 0;
    runtime_ref& session = make_session(kInitialVarCount);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_P(RuntimeParamTest, VacuousSolvedTick) {
    static constexpr size_t kInitialVarCount = 0;
    runtime_ref& session = make_session(kInitialVarCount);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_TRUE(session.derive_decision_lemma().get_resolutions().empty());
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_P(RuntimeParamTest, NormalizeDelegatesToBindMap) {
    static constexpr size_t kInitialVarCount = 2;
    const expr* abc = saved_expr_pool_.make_functor(holder_.functors.id("abc"), {});
    const expr* _123 = saved_expr_pool_.make_functor(holder_.functors.id("123"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {abc, _123}), {}});

    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {
        saved_expr_pool_.make_var(idx_a),
        saved_expr_pool_.make_var(idx_b),
    }));

    runtime_ref& session = make_session(kInitialVarCount);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_EQ(saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_a), 0})),
        abc);
    EXPECT_EQ(saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_b), 0})),
        _123);
    EXPECT_FALSE(session.next()) << "expected refutation";
    EXPECT_TRUE(std::holds_alternative<expr::var>(
        session.normalize({saved_expr_pool_.make_var(idx_a), 0})->content));
}

TEST_P(RuntimeParamTest, ImportSurvivesNextTick) {
    static constexpr size_t kInitialVarCount = 1;
    const expr* abc = saved_expr_pool_.make_functor(holder_.functors.id("abc"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {abc}), {}});

    constexpr uint32_t idx_a = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {saved_expr_pool_.make_var(idx_a)}));

    runtime_ref& session = make_session(kInitialVarCount);
    const expr* kept = nullptr;
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    kept = saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_a), 0}));
    EXPECT_EQ(*kept, *abc);
    EXPECT_FALSE(session.next()) << "expected refutation";
    EXPECT_EQ(*kept, *abc);
}

TEST_P(RuntimeParamTest, DeriveDecisionLemmaOnDemand) {
    static constexpr size_t kInitialVarCount = 0;
    const expr* goal = saved_expr_pool_.make_functor(holder_.functors.id("f"), {});
    const expr* head0 = saved_expr_pool_.make_functor(holder_.functors.id("f"), {});
    const expr* head1 = saved_expr_pool_.make_functor(holder_.functors.id("f"), {});
    initial_goals.push(goal);
    database.push(rule{head0, {}});
    database.push(rule{head1, {}});

    runtime_ref& session = make_session(kInitialVarCount);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_FALSE(session.derive_decision_lemma().get_resolutions().empty());
}

TEST_P(RuntimeParamTest, DeriveResolutionLemmaOnDemand) {
    static constexpr size_t kInitialVarCount = 0;
    const expr* goal = saved_expr_pool_.make_functor(holder_.functors.id("f"), {});
    const expr* head = saved_expr_pool_.make_functor(holder_.functors.id("f"), {});
    initial_goals.push(goal);
    database.push(rule{head, {}});

    runtime_ref& session = make_session(kInitialVarCount);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_FALSE(session.derive_resolution_lemma().get_resolutions().empty());
}

TEST_P(RuntimeParamTest, LemmaNotCachedAcrossTicks) {
    static constexpr size_t kInitialVarCount = 0;
    const expr* goal = saved_expr_pool_.make_functor(holder_.functors.id("f"), {});
    const expr* head0 = saved_expr_pool_.make_functor(holder_.functors.id("f"), {});
    const expr* head1 = saved_expr_pool_.make_functor(holder_.functors.id("f"), {});
    initial_goals.push(goal);
    database.push(rule{head0, {}});
    database.push(rule{head1, {}});

    runtime_ref& session = make_session(kInitialVarCount);

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

TEST_P(RuntimeParamTest, FindsSingleUnitSolution) {
    static constexpr size_t kInitialVarCount = 0;
    const expr* goal = saved_expr_pool_.make_functor(holder_.functors.id("f"), {});
    initial_goals.push(goal);
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});

    runtime_ref& session = make_session(kInitialVarCount);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_TRUE(session.derive_decision_lemma().get_resolutions().empty());
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_P(RuntimeParamTest, RefutesWhenNoCandidates) {
    static constexpr size_t kInitialVarCount = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {}));

    runtime_ref& session = make_session(kInitialVarCount);
    ASSERT_TRUE(session.next()) << "expected a tick";
    EXPECT_FALSE(session.solved()) << "expected conflict termination";
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_P(RuntimeParamTest, FindsClauseDerivedUnitSolution) {
    static constexpr size_t kInitialVarCount = 0;
    const expr* g_body = saved_expr_pool_.make_functor(holder_.functors.id("g"), {});
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {}));
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {g_body}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {}), {}});

    runtime_ref& session = make_session(kInitialVarCount);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_TRUE(session.derive_decision_lemma().get_resolutions().empty());
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_P(RuntimeParamTest, FindsSolutionWithCorrectBindings) {
    static constexpr size_t kInitialVarCount = 2;
    const expr* abc = saved_expr_pool_.make_functor(holder_.functors.id("abc"), {});
    const expr* _123 = saved_expr_pool_.make_functor(holder_.functors.id("123"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {abc, _123}), {}});

    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {
        saved_expr_pool_.make_var(idx_a),
        saved_expr_pool_.make_var(idx_b),
    }));

    runtime_ref& session = make_session(kInitialVarCount);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_EQ(*saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_a), 0})),
        *abc);
    EXPECT_EQ(*saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_b), 0})),
        *_123);
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_P(RuntimeParamTest, FindsClauseBodyBindingSolution) {
    static constexpr size_t kInitialVarCount = 2;
    const expr* rule_var_a = saved_expr_pool_.make_var(0);
    const expr* rule_var_b = saved_expr_pool_.make_var(1);
    const expr* abc = saved_expr_pool_.make_functor(holder_.functors.id("abc"), {});
    const expr* _123 = saved_expr_pool_.make_functor(holder_.functors.id("123"), {});
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("f"), {rule_var_a, rule_var_b}),
        {saved_expr_pool_.make_functor(holder_.functors.id("g"), {rule_var_a}), saved_expr_pool_.make_functor(holder_.functors.id("h"), {rule_var_b})}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {abc}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("h"), {_123}), {}});

    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {
        saved_expr_pool_.make_var(idx_a),
        saved_expr_pool_.make_var(idx_b),
    }));

    runtime_ref& session = make_session(kInitialVarCount);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_EQ(*saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_a), 0})),
        *abc);
    EXPECT_EQ(*saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_b), 0})),
        *_123);
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_P(RuntimeParamTest, RefutesAfterCdclOnUnsatClauseBranches) {
    static constexpr size_t kInitialVarCount = 0;
    const expr* b = saved_expr_pool_.make_functor(holder_.functors.id("b"), {});
    const expr* c = saved_expr_pool_.make_functor(holder_.functors.id("c"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("a"), {}), {b}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("a"), {}), {c}});
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("a"), {}));

    runtime_ref& session = make_session(kInitialVarCount);
    size_t conflict_ticks = 0;
    while (session.next()) {
        EXPECT_FALSE(session.solved());
        ++conflict_ticks;
    }
    ASSERT_GE(conflict_ticks, 2u);
}

// Tier C — enumeration

TEST_P(RuntimeParamTest, EnumeratesTwoGroundChoiceSolutions) {
    static constexpr size_t kInitialVarCount = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {}));
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});

    runtime_ref& session = make_session(kInitialVarCount);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 2u);
}

TEST_P(RuntimeParamTest, RefutesAfterEnumeratingAllGroundBranches) {
    static constexpr size_t kInitialVarCount = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {}));
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});

    runtime_ref& session = make_session(kInitialVarCount);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 2u);
}

TEST_P(RuntimeParamTest, EnumeratesTwoVarChoiceSolutions) {
    static constexpr size_t kInitialVarCount = 1;
    const expr* abc = saved_expr_pool_.make_functor(holder_.functors.id("abc"), {});
    const expr* xyz = saved_expr_pool_.make_functor(holder_.functors.id("xyz"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {abc}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {xyz}), {}});

    constexpr uint32_t idx_a = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {saved_expr_pool_.make_var(idx_a)}));

    runtime_ref& session = make_session(kInitialVarCount);
    std::set<solution> expected{{abc}, {xyz}};
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_a), 0}))};
        });
}

TEST_P(RuntimeParamTest, RefutesAfterEnumeratingAllVarBranches) {
    static constexpr size_t kInitialVarCount = 1;
    const expr* abc = saved_expr_pool_.make_functor(holder_.functors.id("abc"), {});
    const expr* xyz = saved_expr_pool_.make_functor(holder_.functors.id("xyz"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {abc}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {xyz}), {}});

    constexpr uint32_t idx_a = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {saved_expr_pool_.make_var(idx_a)}));

    runtime_ref& session = make_session(kInitialVarCount);
    std::set<solution> expected{{abc}, {xyz}};
    next_until_refuted(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_a), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesTwoGoalSharedVarSolutions) {
    static constexpr size_t kInitialVarCount = 1;
    const expr* abc = saved_expr_pool_.make_functor(holder_.functors.id("abc"), {});
    const expr* xyz = saved_expr_pool_.make_functor(holder_.functors.id("xyz"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {abc}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {xyz}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {abc}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {xyz}), {}});

    constexpr uint32_t idx_a = 0;
    const expr* goal_var = saved_expr_pool_.make_var(idx_a);
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {goal_var}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("g"), {goal_var}));

    runtime_ref& session = make_session(kInitialVarCount);
    std::set<solution> expected{{abc}, {xyz}};
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_a), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesFourVarBindingSolutions) {
    static constexpr size_t kInitialVarCount = 1;
    const expr* a = saved_expr_pool_.make_functor(holder_.functors.id("a"), {});
    const expr* b = saved_expr_pool_.make_functor(holder_.functors.id("b"), {});
    const expr* c = saved_expr_pool_.make_functor(holder_.functors.id("c"), {});
    const expr* d = saved_expr_pool_.make_functor(holder_.functors.id("d"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {a}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {b}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {c}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {d}), {}});

    constexpr uint32_t idx = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {saved_expr_pool_.make_var(idx)}));

    runtime_ref& session = make_session(kInitialVarCount);
    std::set<solution> expected{{a}, {b}, {c}, {d}};
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesFourClauseBodyFactChoices) {
    static constexpr size_t kInitialVarCount = 0;
    const expr* rule_var = saved_expr_pool_.make_var(0);
    const expr* a = saved_expr_pool_.make_functor(holder_.functors.id("a"), {});
    const expr* b = saved_expr_pool_.make_functor(holder_.functors.id("b"), {});
    const expr* c = saved_expr_pool_.make_functor(holder_.functors.id("c"), {});
    const expr* d = saved_expr_pool_.make_functor(holder_.functors.id("d"), {});
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("f"), {}),
        {saved_expr_pool_.make_functor(holder_.functors.id("g"), {rule_var})}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {a}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {b}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {c}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {d}), {}});
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {}));

    runtime_ref& session = make_session(kInitialVarCount);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 4u);
}

// Tier E

TEST_P(RuntimeParamTest, FindsUniqueSharedVarConjunctionThenRefutes) {
    static constexpr size_t kInitialVarCount = 1;
    const expr* one = saved_expr_pool_.make_functor(holder_.functors.id("1"), {});
    const expr* two = saved_expr_pool_.make_functor(holder_.functors.id("2"), {});
    const expr* three = saved_expr_pool_.make_functor(holder_.functors.id("3"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("is_a"), {one}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("is_a"), {two}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("is_b"), {two}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("is_b"), {three}), {}});

    constexpr uint32_t idx_x = 0;
    const expr* goal_var = saved_expr_pool_.make_var(idx_x);
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("is_a"), {goal_var}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("is_b"), {goal_var}));

    runtime_ref& session = make_session(kInitialVarCount);
    std::set<solution> expected{{two}};
    next_until_refuted(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_x), 0}))};
        });
}

// Tier F

TEST_P(RuntimeParamTest, ConflictedTickNotSolved) {
    static constexpr size_t kInitialVarCount = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {}));

    runtime_ref& session = make_session(kInitialVarCount);
    ASSERT_TRUE(session.next()) << "expected a tick";
    EXPECT_FALSE(session.solved()) << "expected conflict termination";
    EXPECT_FALSE(session.next()) << "expected refutation";
}

TEST_P(RuntimeParamTest, SkippedLemmaCallsOnUnitTicks) {
    static constexpr size_t kInitialVarCount = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {}));
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});

    runtime_ref& session = make_session(kInitialVarCount);
    ASSERT_TRUE(session.next()) << "expected a tick";
    ASSERT_TRUE(session.solved()) << "expected solved termination";
    EXPECT_FALSE(session.next()) << "expected refutation";
}

// Tier G — CHC enumeration via basic_runtime

TEST_P(RuntimeParamTest, EnumeratesTwoParentBindingsForAlice) {
    static constexpr size_t kInitialVarCount = 1;
    const expr* bob = saved_expr_pool_.make_functor(holder_.functors.id("bob"), {});
    const expr* carol = saved_expr_pool_.make_functor(holder_.functors.id("carol"), {});
    const expr* alice = saved_expr_pool_.make_functor(holder_.functors.id("alice"), {});
    const expr* dave = saved_expr_pool_.make_functor(holder_.functors.id("dave"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("parent"), {bob, alice}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("parent"), {carol, alice}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("parent"), {dave, bob}), {}});
    constexpr uint32_t idx_x = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("parent"), {
        saved_expr_pool_.make_var(idx_x),
        alice,
    }));

    runtime_ref& session = make_session(kInitialVarCount);
    std::set<solution> expected{{bob}, {carol}};
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_x), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesPeanoLessThanSeven) {
    static constexpr size_t kInitialVarCount = 1;
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {zero, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv2})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});

    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv4})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("lt"), {rv3, rv4})}});

    std::set<solution> expected;
    for (int n = 0; n < 7; ++n)
        expected.insert({peano_saved(n)});
    constexpr uint32_t idx_n = 0;
    const expr* seven = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    for (int i = 0; i < 7; ++i)
        seven = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {seven});
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_n)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("lt"), {
        saved_expr_pool_.make_var(idx_n),
        seven,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kPeanoBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_n), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesSatPAndQOrR) {
    static constexpr size_t kInitialVarCount = 4;
    const expr* true_atom = saved_expr_pool_.make_functor(holder_.functors.id("true"), {});
    const expr* false_atom = saved_expr_pool_.make_functor(holder_.functors.id("false"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("bool"), {true_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("bool"), {false_atom}), {}});

    const expr* or_rv = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("or"), {true_atom, or_rv, true_atom}),
        {saved_expr_pool_.make_functor(holder_.functors.id("bool"), {or_rv})}});

    const expr* or_rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("or"), {false_atom, or_rv2, or_rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("bool"), {or_rv2})}});

    const expr* and_rv = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("and"), {true_atom, and_rv, and_rv}),
        {saved_expr_pool_.make_functor(holder_.functors.id("bool"), {and_rv})}});

    const expr* and_rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("and"), {false_atom, and_rv2, false_atom}),
        {saved_expr_pool_.make_functor(holder_.functors.id("bool"), {and_rv2})}});
    constexpr uint32_t idx_p = 0;
    constexpr uint32_t idx_q = 1;
    constexpr uint32_t idx_r = 2;
    constexpr uint32_t idx_qr = 3;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("bool"), {saved_expr_pool_.make_var(idx_p)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("bool"), {saved_expr_pool_.make_var(idx_q)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("bool"), {saved_expr_pool_.make_var(idx_r)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("or"), {
        saved_expr_pool_.make_var(idx_q),
        saved_expr_pool_.make_var(idx_r),
        saved_expr_pool_.make_var(idx_qr),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("and"), {
        saved_expr_pool_.make_var(idx_p),
        saved_expr_pool_.make_var(idx_qr),
        saved_expr_pool_.make_functor(holder_.functors.id("true"), {}),
    }));

    runtime_ref& session = make_session(kInitialVarCount);
    std::set<solution> expected{
        {true_atom, true_atom, true_atom},
        {true_atom, true_atom, false_atom},
        {true_atom, false_atom, true_atom},
    };
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_p), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_q), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_r), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesTwoSatAssignmentsForImpliesQ) {
    static constexpr size_t kInitialVarCount = 3;
    const expr* true_atom = saved_expr_pool_.make_functor(holder_.functors.id("true"), {});
    const expr* false_atom = saved_expr_pool_.make_functor(holder_.functors.id("false"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("bool"), {true_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("bool"), {false_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("not"), {true_atom, false_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("not"), {false_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("or"), {true_atom, true_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("or"), {true_atom, false_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("or"), {false_atom, true_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("or"), {false_atom, false_atom, false_atom}), {}});
    constexpr uint32_t idx_p = 0;
    constexpr uint32_t idx_q = 1;
    constexpr uint32_t idx_np = 2;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("bool"), {saved_expr_pool_.make_var(idx_p)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("bool"), {saved_expr_pool_.make_var(idx_q)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("or"), {
        saved_expr_pool_.make_var(idx_p),
        saved_expr_pool_.make_var(idx_q),
        saved_expr_pool_.make_functor(holder_.functors.id("true"), {}),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("not"), {
        saved_expr_pool_.make_var(idx_p),
        saved_expr_pool_.make_var(idx_np),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("or"), {
        saved_expr_pool_.make_var(idx_np),
        saved_expr_pool_.make_var(idx_q),
        saved_expr_pool_.make_functor(holder_.functors.id("true"), {}),
    }));

    runtime_ref& session = make_session(kInitialVarCount);
    std::set<std::string> p_values;
    size_t solution_count = 0;
    while (session.next()) {
        if (!session.solved())
            continue;
        ++solution_count;
        const expr* q_val = session.normalize({saved_expr_pool_.make_var(idx_q), 0});
        const expr* p_val = session.normalize({saved_expr_pool_.make_var(idx_p), 0});
        ASSERT_TRUE(std::holds_alternative<expr::functor>(q_val->content));
        ASSERT_TRUE(std::holds_alternative<expr::functor>(p_val->content));
        EXPECT_EQ(std::get<expr::functor>(q_val->content).id, holder_.functors.id("true"));
        p_values.insert(holder_.functors.names.name(std::get<expr::functor>(p_val->content).id));
        if (solution_count >= 2u)
            break;
    }
    EXPECT_EQ(solution_count, 2u);
    EXPECT_EQ(p_values.size(), 2u);
}

TEST_P(RuntimeParamTest, EnumeratesTwoPathTwoColorings) {
    static constexpr size_t kInitialVarCount = 3;
    const expr* red = saved_expr_pool_.make_functor(holder_.functors.id("red"), {});
    const expr* blue = saved_expr_pool_.make_functor(holder_.functors.id("blue"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("color"), {red}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("color"), {blue}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("diff"), {red, blue}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("diff"), {blue, red}), {}});
    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    constexpr uint32_t idx_c = 2;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("color"), {saved_expr_pool_.make_var(idx_a)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("color"), {saved_expr_pool_.make_var(idx_b)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("color"), {saved_expr_pool_.make_var(idx_c)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("diff"), {
        saved_expr_pool_.make_var(idx_a),
        saved_expr_pool_.make_var(idx_b),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("diff"), {
        saved_expr_pool_.make_var(idx_b),
        saved_expr_pool_.make_var(idx_c),
    }));

    runtime_ref& session = make_session(kInitialVarCount);
    auto is_valid_color = [](const std::string& s) {
        return s == "red" || s == "blue";
    };

    std::string a1;
    size_t found = 0;
    while (session.next()) {
        if (!session.solved())
            continue;
        const std::string a_str =
            holder_.functors.names.name(std::get<expr::functor>(session.normalize({saved_expr_pool_.make_var(idx_a), 0})->content).id);
        const std::string b_str =
            holder_.functors.names.name(std::get<expr::functor>(session.normalize({saved_expr_pool_.make_var(idx_b), 0})->content).id);
        const std::string c_str =
            holder_.functors.names.name(std::get<expr::functor>(session.normalize({saved_expr_pool_.make_var(idx_c), 0})->content).id);
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
TEST_P(RuntimeParamTest, EnumeratesK3ThreeColorings) {
    static constexpr size_t kInitialVarCount = 3;
    static constexpr size_t kColorBudget = 128;

    const expr* red = saved_expr_pool_.make_functor(holder_.functors.id("red"), {});
    const expr* green = saved_expr_pool_.make_functor(holder_.functors.id("green"), {});
    const expr* blue = saved_expr_pool_.make_functor(holder_.functors.id("blue"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("color"), {red}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("color"), {green}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("color"), {blue}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("diff"), {red, green}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("diff"), {red, blue}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("diff"), {green, red}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("diff"), {green, blue}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("diff"), {blue, red}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("diff"), {blue, green}), {}});
    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    constexpr uint32_t idx_c = 2;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("color"), {saved_expr_pool_.make_var(idx_a)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("color"), {saved_expr_pool_.make_var(idx_b)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("color"), {saved_expr_pool_.make_var(idx_c)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("diff"), {
        saved_expr_pool_.make_var(idx_a),
        saved_expr_pool_.make_var(idx_b),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("diff"), {
        saved_expr_pool_.make_var(idx_a),
        saved_expr_pool_.make_var(idx_c),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("diff"), {
        saved_expr_pool_.make_var(idx_b),
        saved_expr_pool_.make_var(idx_c),
    }));

    runtime_ref& session = make_session(kInitialVarCount, kColorBudget);
    std::set<solution> expected = {
        {red, green, blue}, {red, blue, green}, {green, red, blue},
        {green, blue, red}, {blue, red, green}, {blue, green, red},
    };
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_a), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_b), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_c), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesK3TailFourNodeColorings) {
    static constexpr size_t kInitialVarCount = 4;
    static constexpr size_t kColorBudget = 128;

    const expr* red = saved_expr_pool_.make_functor(holder_.functors.id("red"), {});
    const expr* green = saved_expr_pool_.make_functor(holder_.functors.id("green"), {});
    const expr* blue = saved_expr_pool_.make_functor(holder_.functors.id("blue"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("color"), {red}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("color"), {green}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("color"), {blue}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("diff"), {red, green}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("diff"), {red, blue}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("diff"), {green, red}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("diff"), {green, blue}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("diff"), {blue, red}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("diff"), {blue, green}), {}});
    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    constexpr uint32_t idx_c = 2;
    constexpr uint32_t idx_d = 3;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("color"), {saved_expr_pool_.make_var(idx_a)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("color"), {saved_expr_pool_.make_var(idx_b)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("color"), {saved_expr_pool_.make_var(idx_c)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("color"), {saved_expr_pool_.make_var(idx_d)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("diff"), {
        saved_expr_pool_.make_var(idx_a),
        saved_expr_pool_.make_var(idx_b),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("diff"), {
        saved_expr_pool_.make_var(idx_a),
        saved_expr_pool_.make_var(idx_c),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("diff"), {
        saved_expr_pool_.make_var(idx_b),
        saved_expr_pool_.make_var(idx_c),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("diff"), {
        saved_expr_pool_.make_var(idx_a),
        saved_expr_pool_.make_var(idx_d),
    }));

    runtime_ref& session = make_session(kInitialVarCount, kColorBudget);
    std::set<solution> expected = {
        {red, green, blue, green}, {red, green, blue, blue}, {red, blue, green, green},
        {red, blue, green, blue}, {green, red, blue, red}, {green, red, blue, blue},
        {green, blue, red, red}, {green, blue, red, blue}, {blue, red, green, red},
        {blue, red, green, green}, {blue, green, red, red}, {blue, green, red, green},
    };
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_a), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_b), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_c), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_d), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesFourVarSatThreeClauses) {
    static constexpr size_t kInitialVarCount = 10;
    static constexpr size_t kSatBudget = 256;

    const expr* true_atom = saved_expr_pool_.make_functor(holder_.functors.id("true"), {});
    const expr* false_atom = saved_expr_pool_.make_functor(holder_.functors.id("false"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("bool"), {true_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("bool"), {false_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("not"), {true_atom, false_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("not"), {false_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("or"), {true_atom, true_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("or"), {true_atom, false_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("or"), {false_atom, true_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("or"), {false_atom, false_atom, false_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("and"), {true_atom, true_atom, true_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("and"), {true_atom, false_atom, false_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("and"), {false_atom, true_atom, false_atom}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("and"), {false_atom, false_atom, false_atom}), {}});
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
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("bool"), {saved_expr_pool_.make_var(idx_p)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("bool"), {saved_expr_pool_.make_var(idx_q)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("bool"), {saved_expr_pool_.make_var(idx_r)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("bool"), {saved_expr_pool_.make_var(idx_s)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("or"), {
        saved_expr_pool_.make_var(idx_p),
        saved_expr_pool_.make_var(idx_q),
        saved_expr_pool_.make_var(idx_pq),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("or"), {
        saved_expr_pool_.make_var(idx_r),
        saved_expr_pool_.make_var(idx_s),
        saved_expr_pool_.make_var(idx_rs),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("not"), {
        saved_expr_pool_.make_var(idx_p),
        saved_expr_pool_.make_var(idx_np),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("not"), {
        saved_expr_pool_.make_var(idx_r),
        saved_expr_pool_.make_var(idx_nr),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("or"), {
        saved_expr_pool_.make_var(idx_np),
        saved_expr_pool_.make_var(idx_nr),
        saved_expr_pool_.make_var(idx_npr),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("and"), {
        saved_expr_pool_.make_var(idx_pq),
        saved_expr_pool_.make_var(idx_rs),
        saved_expr_pool_.make_var(idx_pq_rs),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("and"), {
        saved_expr_pool_.make_var(idx_pq_rs),
        saved_expr_pool_.make_var(idx_npr),
        saved_expr_pool_.make_functor(holder_.functors.id("true"), {}),
    }));

    runtime_ref& session = make_session(kInitialVarCount, kSatBudget);
    std::set<solution> expected = {
        {true_atom, true_atom, false_atom, true_atom},
        {true_atom, false_atom, false_atom, true_atom},
        {false_atom, true_atom, true_atom, true_atom},
        {false_atom, true_atom, true_atom, false_atom},
        {false_atom, true_atom, false_atom, true_atom},
    };
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_p), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_q), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_r), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_s), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesAddPairsSummingLessThanTen) {
    static constexpr size_t kInitialVarCount = 3;
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv2, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});

    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    const expr* rv5 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), rv4, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {zero, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv6})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv6})}});

    const expr* rv7 = saved_expr_pool_.make_var(0);
    const expr* rv8 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv7}), saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv8})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("lt"), {rv7, rv8})}});

    std::set<solution> expected;
    for (int x = 0; x < 10; ++x) {
        for (int y = 0; y < 10 - x; ++y)
            expected.insert({peano_saved(x), peano_saved(y)});
    }
    ASSERT_EQ(expected.size(), 55u);
    constexpr uint32_t idx_x = 0;
    constexpr uint32_t idx_y = 1;
    constexpr uint32_t idx_s = 2;
    const expr* ten = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    for (int i = 0; i < 10; ++i)
        ten = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {ten});
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("add"), {
        saved_expr_pool_.make_var(idx_x),
        saved_expr_pool_.make_var(idx_y),
        saved_expr_pool_.make_var(idx_s),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("lt"), {
        saved_expr_pool_.make_var(idx_s),
        ten,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kPeanoBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_x), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_y), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesAddPairsSummingExactlyTen) {
    static constexpr size_t kInitialVarCount = 2;
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv2, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});

    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    const expr* rv5 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), rv4, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv3, rv4, rv5})}});

    std::set<solution> expected;
    for (int x = 0; x <= 10; ++x)
        expected.insert({peano_saved(x), peano_saved(10 - x)});
    ASSERT_EQ(expected.size(), 11u);
    constexpr uint32_t idx_x = 0;
    constexpr uint32_t idx_y = 1;
    const expr* ten = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    for (int i = 0; i < 10; ++i)
        ten = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {ten});
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_x)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_y)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("add"), {
        saved_expr_pool_.make_var(idx_x),
        saved_expr_pool_.make_var(idx_y),
        ten,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kPeanoBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_x), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_y), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesMulPairsProductEight) {
    static constexpr size_t kInitialVarCount = 2;
    static constexpr size_t kPeanoBudget = 256;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv2, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});

    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    const expr* rv5 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), rv4, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("mul"), {zero, rv6, zero}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv6})}});

    const expr* rv7 = saved_expr_pool_.make_var(0);
    const expr* rv8 = saved_expr_pool_.make_var(1);
    const expr* rv9 = saved_expr_pool_.make_var(2);
    const expr* rv10 = saved_expr_pool_.make_var(3);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("mul"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv7}), rv8, rv9}),
        {saved_expr_pool_.make_functor(holder_.functors.id("mul"), {rv7, rv8, rv10}),
            saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv10, rv8, rv9})}});

    std::set<solution> expected = {
        {peano_saved(1), peano_saved(8)},
        {peano_saved(2), peano_saved(4)},
        {peano_saved(4), peano_saved(2)},
        {peano_saved(8), peano_saved(1)},
    };
    constexpr uint32_t idx_x = 0;
    constexpr uint32_t idx_y = 1;
    const expr* eight = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    for (int i = 0; i < 8; ++i)
        eight = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {eight});
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_x)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_y)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("mul"), {
        saved_expr_pool_.make_var(idx_x),
        saved_expr_pool_.make_var(idx_y),
        eight,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kPeanoBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_x), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_y), 0}))};
        });
}
TEST_P(RuntimeParamTest, EnumeratesDualBoundedSharedXSums) {
    static constexpr size_t kInitialVarCount = 5;
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv2, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});

    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    const expr* rv5 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), rv4, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {zero, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv6})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv6})}});

    const expr* rv7 = saved_expr_pool_.make_var(0);
    const expr* rv8 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv7}), saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv8})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("lt"), {rv7, rv8})}});

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
    const expr* bound = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    for (int i = 0; i < 4; ++i)
        bound = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {bound});
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_x)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_y)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_z)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("add"), {
        saved_expr_pool_.make_var(idx_x),
        saved_expr_pool_.make_var(idx_y),
        saved_expr_pool_.make_var(idx_s),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("add"), {
        saved_expr_pool_.make_var(idx_x),
        saved_expr_pool_.make_var(idx_z),
        saved_expr_pool_.make_var(idx_t),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("lt"), {
        saved_expr_pool_.make_var(idx_s),
        bound,
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("lt"), {
        saved_expr_pool_.make_var(idx_t),
        bound,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kPeanoBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_x), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_y), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_z), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesCatalanTreesWithFiveNodes) {
    static constexpr size_t kInitialVarCount = 1;
    static constexpr size_t kCatalanBudget = 70;

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv2, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});

    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    const expr* rv5 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), rv4, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv3, rv4, rv5})}});

    const expr* nil = saved_expr_pool_.make_functor(holder_.functors.id("nil"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("wf"), {nil}), {}});

    const expr* rv6 = saved_expr_pool_.make_var(0);
    const expr* rv7 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("wf"), {saved_expr_pool_.make_functor(holder_.functors.id("bin"), {rv6, rv7})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("wf"), {rv6}), saved_expr_pool_.make_functor(holder_.functors.id("wf"), {rv7})}});

    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nodes"), {nil, zero}), {}});

    const expr* one = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {zero});
    const expr* rv8 = saved_expr_pool_.make_var(0);
    const expr* rv9 = saved_expr_pool_.make_var(1);
    const expr* rv10 = saved_expr_pool_.make_var(2);
    const expr* rv11 = saved_expr_pool_.make_var(3);
    const expr* rv12 = saved_expr_pool_.make_var(4);
    const expr* rv13 = saved_expr_pool_.make_var(5);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nodes"), {saved_expr_pool_.make_functor(holder_.functors.id("bin"), {rv8, rv9}), rv10}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nodes"), {rv8, rv11}),
            saved_expr_pool_.make_functor(holder_.functors.id("nodes"), {rv9, rv12}),
            saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv11, rv12, rv13}),
            saved_expr_pool_.make_functor(holder_.functors.id("add"), {one, rv13, rv10})}});

    constexpr uint32_t idx_t = 0;
    const expr* five = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    for (int i = 0; i < 5; ++i)
        five = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {five});
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("wf"), {saved_expr_pool_.make_var(idx_t)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nodes"), {saved_expr_pool_.make_var(idx_t), five}));

    runtime_ref& session = make_session(kInitialVarCount, kCatalanBudget);
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_t), 0}))};
        if (visited.count(s))
            continue;
        visited.insert(s);
    }
    EXPECT_EQ(visited.size(), 42u);
}

TEST_P(RuntimeParamTest, EnumeratesFourTwoGoalGroundCombinations) {
    static constexpr size_t kInitialVarCount = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("g"), {}));
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {}), {}});

    runtime_ref& session = make_session(kInitialVarCount);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 4u);
}

TEST_P(RuntimeParamTest, EnumeratesEightThreeGoalGroundCombinations) {
    static constexpr size_t kInitialVarCount = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("g"), {}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("h"), {}));
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("h"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("h"), {}), {}});

    runtime_ref& session = make_session(kInitialVarCount);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 8u);
}

TEST_P(RuntimeParamTest, EnumeratesManySharedVarGroundHeads) {
    static constexpr size_t kInitialVarCount = 3;
    const expr* abc = saved_expr_pool_.make_functor(holder_.functors.id("abc"), {});
    const expr* def = saved_expr_pool_.make_functor(holder_.functors.id("def"), {});
    const expr* ghi = saved_expr_pool_.make_functor(holder_.functors.id("ghi"), {});
    const expr* jkl = saved_expr_pool_.make_functor(holder_.functors.id("jkl"), {});
    const expr* mno = saved_expr_pool_.make_functor(holder_.functors.id("mno"), {});
    const expr* pqr = saved_expr_pool_.make_functor(holder_.functors.id("pqr"), {});
    const expr* xyz = saved_expr_pool_.make_functor(holder_.functors.id("xyz"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {abc, xyz, pqr}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {def, xyz, pqr}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {ghi, xyz, pqr}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {jkl, xyz, pqr}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {mno, xyz, pqr}), {}});
    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    constexpr uint32_t idx_c = 2;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("g"), {
        saved_expr_pool_.make_var(idx_a),
        saved_expr_pool_.make_var(idx_b),
        saved_expr_pool_.make_var(idx_c),
    }));

    runtime_ref& session = make_session(kInitialVarCount);
    std::set<solution> visited;
    while (session.next()) {
        if (!session.solved())
            continue;
        const solution s = {
            saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_a), 0})),
            saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_b), 0})),
            saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_c), 0})),
        };
        if (visited.count(s))
            continue;
        visited.insert(s);
        ASSERT_EQ(*s[1], *xyz);
        ASSERT_EQ(*s[2], *pqr);
    }
    EXPECT_EQ(visited.size(), 5u);
}

TEST_P(RuntimeParamTest, EnumeratesThreeGroundBranches) {
    static constexpr size_t kInitialVarCount = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {}));
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {}});

    runtime_ref& session = make_session(kInitialVarCount);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 3u);
}

TEST_P(RuntimeParamTest, SolvesRecursiveClauseTreeWithoutBranching) {
    static constexpr size_t kInitialVarCount = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {}));
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("f"), {}), {
        saved_expr_pool_.make_functor(holder_.functors.id("g"), {}),
        saved_expr_pool_.make_functor(holder_.functors.id("h"), {}),
    }});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {}), {
        saved_expr_pool_.make_functor(holder_.functors.id("i"), {}),
        saved_expr_pool_.make_functor(holder_.functors.id("j"), {}),
    }});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("h"), {}), {
        saved_expr_pool_.make_functor(holder_.functors.id("i"), {}),
        saved_expr_pool_.make_functor(holder_.functors.id("j"), {}),
    }});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("i"), {}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("j"), {}), {}});

    runtime_ref& session = make_session(kInitialVarCount);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 1u);
}

TEST_P(RuntimeParamTest, EnumeratesTransitiveReachFromA) {
    static constexpr size_t kInitialVarCount = 1;
    const expr* a = saved_expr_pool_.make_functor(holder_.functors.id("a"), {});
    const expr* b = saved_expr_pool_.make_functor(holder_.functors.id("b"), {});
    const expr* c = saved_expr_pool_.make_functor(holder_.functors.id("c"), {});
    const expr* d = saved_expr_pool_.make_functor(holder_.functors.id("d"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("reach"), {a, b}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("reach"), {a, c}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("reach"), {a, d}), {}});

    constexpr uint32_t idx_y = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("reach"), {a, saved_expr_pool_.make_var(idx_y)}));

    runtime_ref& session = make_session(kInitialVarCount);
    std::set<solution> expected{{b}, {c}, {d}};
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_y), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesEvenPeanoLessThanEight) {
    static constexpr size_t kInitialVarCount = 1;
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("even"), {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    const expr* suc_suc_rv2 = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv2})});
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("even"), {suc_suc_rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("even"), {rv2})}});

    const expr* rv3 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {zero, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv3})}});

    const expr* rv4 = saved_expr_pool_.make_var(0);
    const expr* rv5 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv4}), saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("lt"), {rv4, rv5})}});

    std::set<solution> expected{
        {peano_saved(0)},
        {peano_saved(2)},
        {peano_saved(4)},
        {peano_saved(6)},
    };
    constexpr uint32_t idx_n = 0;
    const expr* eight = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    for (int i = 0; i < 8; ++i)
        eight = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {eight});
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("even"), {saved_expr_pool_.make_var(idx_n)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("lt"), {
        saved_expr_pool_.make_var(idx_n),
        eight,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kPeanoBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_n), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesListSplitsForThreeElementList) {
    static constexpr size_t kInitialVarCount = 2;
    const expr* nil = saved_expr_pool_.make_functor(holder_.functors.id("nil"), {});
    const expr* a = saved_expr_pool_.make_functor(holder_.functors.id("a"), {});
    const expr* b = saved_expr_pool_.make_functor(holder_.functors.id("b"), {});
    const expr* list_ab = saved_expr_pool_.make_functor(holder_.functors.id("cons"), {a, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {b, nil})});
    const expr* list_a = saved_expr_pool_.make_functor(holder_.functors.id("cons"), {a, nil});
    const expr* list_b = saved_expr_pool_.make_functor(holder_.functors.id("cons"), {b, nil});

    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("append"), {nil, list_ab, list_ab}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("append"), {list_a, list_b, list_ab}), {}});

    constexpr uint32_t idx_l1 = 0;
    constexpr uint32_t idx_l2 = 1;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("append"), {
        saved_expr_pool_.make_var(idx_l1),
        saved_expr_pool_.make_var(idx_l2),
        list_ab,
    }));

    runtime_ref& session = make_session(kInitialVarCount);
    std::set<solution> expected{{nil, list_ab}, {list_a, list_b}};
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_l1), 0})), saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_l2), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesTwoChoiceClauseSolutions) {
    static constexpr size_t kInitialVarCount = 0;
    const expr* abc = saved_expr_pool_.make_functor(holder_.functors.id("abc"), {});
    const expr* xyz = saved_expr_pool_.make_functor(holder_.functors.id("xyz"), {});
    const expr* rule_var = saved_expr_pool_.make_var(0);
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("f"), {}));
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("f"), {}),
        {saved_expr_pool_.make_functor(holder_.functors.id("g"), {rule_var})}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {abc}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("g"), {xyz}), {}});

    runtime_ref& session = make_session(kInitialVarCount);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, 2u);
}

// Tier H — novel CHC synthesis

TEST_P(RuntimeParamTest, EnumeratesCollatzOneStepPreimagesOfTen) {
    static constexpr size_t kInitialVarCount = 1;
    /*
     * Intent: inverse branching on the Collatz step relation.
     * step(N, M) — if even(N) then M = N/2; if odd(N) then M = 3N+1.
     * Goal: step(N, ten). One-step preimages of 10 are N=3 (3*3+1) and N=20 (20/2).
     */
    static constexpr size_t kCollatzBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    const expr* one = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {zero});
    const expr* two = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {one});
    const expr* three = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {two});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("even"), {zero}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("odd"), {one}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});
    const expr* rv2 = saved_expr_pool_.make_var(0);
    const expr* suc_suc_rv2 = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv2})});
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("even"), {suc_suc_rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("even"), {rv2})}});
    const expr* rv3 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("odd"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("even"), {rv3})}});

    const expr* rv4 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv4, rv4}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv4})}});
    const expr* rv5 = saved_expr_pool_.make_var(0);
    const expr* rv6 = saved_expr_pool_.make_var(1);
    const expr* rv7 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5}), rv6, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv7})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv5, rv6, rv7})}});

    const expr* rv8 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("mul"), {zero, rv8, zero}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv8})}});
    const expr* rv9 = saved_expr_pool_.make_var(0);
    const expr* rv10 = saved_expr_pool_.make_var(1);
    const expr* rv11 = saved_expr_pool_.make_var(2);
    const expr* rv11b = saved_expr_pool_.make_var(3);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("mul"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv9}), rv10, rv11}),
        {saved_expr_pool_.make_functor(holder_.functors.id("mul"), {rv9, rv10, rv11b}),
            saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv11b, rv10, rv11})}});

    const expr* rv12 = saved_expr_pool_.make_var(0);
    const expr* rv13 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("step"), {rv12, rv13}),
        {saved_expr_pool_.make_functor(holder_.functors.id("even"), {rv12}), saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv13, rv13, rv12})}});
    const expr* rv14 = saved_expr_pool_.make_var(0);
    const expr* rv15 = saved_expr_pool_.make_var(1);
    const expr* rv16 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("step"), {rv14, rv16}),
        {saved_expr_pool_.make_functor(holder_.functors.id("odd"), {rv14}),
            saved_expr_pool_.make_functor(holder_.functors.id("mul"), {three, rv14, rv15}),
            saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv15, one, rv16})}});

    const expr* ten = peano_saved(10);
    constexpr uint32_t idx_n = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_n)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("step"), {
        saved_expr_pool_.make_var(idx_n),
        ten,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kCollatzBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        {{peano_saved(3)}, {peano_saved(20)}},
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_n), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesFibIndicesWithValueBelowFive) {
    static constexpr size_t kInitialVarCount = 2;
    /*
     * Intent: recursive fib index synthesis bounded by output value.
     * fib(0)=0, fib(1)=1, fib(n+2)=fib(n)+fib(n+1).
     * Goal: fib(N, V), lt(V, five) — all indices with fib value below 5.
     */
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    const expr* one = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {zero});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("fib"), {zero, zero}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("fib"), {one, one}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv2, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});
    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    const expr* rv5 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), rv4, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make_var(0);
    const expr* rv7 = saved_expr_pool_.make_var(1);
    const expr* rv8 = saved_expr_pool_.make_var(2);
    const expr* rv9 = saved_expr_pool_.make_var(3);
    const expr* suc_rv6 = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv6});
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("fib"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {suc_rv6}), rv9}),
        {saved_expr_pool_.make_functor(holder_.functors.id("fib"), {rv6, rv7}),
            saved_expr_pool_.make_functor(holder_.functors.id("fib"), {suc_rv6, rv8}),
            saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv7, rv8, rv9})}});

    const expr* rv10 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {zero, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv10})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv10})}});
    const expr* rv11 = saved_expr_pool_.make_var(0);
    const expr* rv12 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv11}), saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv12})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("lt"), {rv11, rv12})}});

    std::set<solution> expected{
        {peano_saved(0)},
        {peano_saved(1)},
        {peano_saved(2)},
        {peano_saved(3)},
        {peano_saved(4)},
    };
    constexpr uint32_t idx_n = 0;
    constexpr uint32_t idx_v = 1;
    const expr* five = peano_saved(5);
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_n)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("fib"), {
        saved_expr_pool_.make_var(idx_n),
        saved_expr_pool_.make_var(idx_v),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("lt"), {
        saved_expr_pool_.make_var(idx_v),
        five,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kPeanoBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_n), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesFactorPairsOfSix) {
    static constexpr size_t kInitialVarCount = 2;
    /*
     * Intent: commutative mul synthesis — (X,Y) and (Y,X) are distinct models.
     * Goal: mul(X, Y, six). Expected 4 factor pairs: (1,6), (6,1), (2,3), (3,2).
     */
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv2, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});
    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    const expr* rv5 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), rv4, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("mul"), {zero, rv6, zero}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv6})}});
    const expr* rv7 = saved_expr_pool_.make_var(0);
    const expr* rv8 = saved_expr_pool_.make_var(1);
    const expr* rv9 = saved_expr_pool_.make_var(2);
    const expr* rv10 = saved_expr_pool_.make_var(3);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("mul"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv7}), rv8, rv9}),
        {saved_expr_pool_.make_functor(holder_.functors.id("mul"), {rv7, rv8, rv10}),
            saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv10, rv8, rv9})}});

    std::set<solution> expected{
        {peano_saved(1), peano_saved(6)},
        {peano_saved(6), peano_saved(1)},
        {peano_saved(2), peano_saved(3)},
        {peano_saved(3), peano_saved(2)},
    };
    constexpr uint32_t idx_x = 0;
    constexpr uint32_t idx_y = 1;
    const expr* six = peano_saved(6);
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_x)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_y)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("mul"), {
        saved_expr_pool_.make_var(idx_x),
        saved_expr_pool_.make_var(idx_y),
        six,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kPeanoBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {
                saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_x), 0})),
                saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_y), 0})),
            };
        });
}

TEST_P(RuntimeParamTest, EnumeratesDistinctTwoPartPartitionsOfFive) {
    static constexpr size_t kInitialVarCount = 2;
    /*
     * Intent: partition 5 into two distinct positive parts with A < B.
     * Goal: part(five, A, B). Expected: (1,4) and (2,3).
     */
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv2, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});
    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    const expr* rv5 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), rv4, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {zero, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv6})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv6})}});
    const expr* rv7 = saved_expr_pool_.make_var(0);
    const expr* rv8 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv7}), saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv8})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("lt"), {rv7, rv8})}});

    const expr* rv9 = saved_expr_pool_.make_var(0);
    const expr* rv10 = saved_expr_pool_.make_var(1);
    const expr* five = peano_saved(5);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("part"), {five, rv9, rv10}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv9, rv10, five}),
            saved_expr_pool_.make_functor(holder_.functors.id("lt"), {zero, rv9}),
            saved_expr_pool_.make_functor(holder_.functors.id("lt"), {rv9, rv10})}});

    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_b = 1;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("part"), {
        five,
        saved_expr_pool_.make_var(idx_a),
        saved_expr_pool_.make_var(idx_b),
    }));

    runtime_ref& session = make_session(kInitialVarCount, kPeanoBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        {{peano_saved(1), peano_saved(4)}, {peano_saved(2), peano_saved(3)}},
        [&]() -> solution {
            return {
                saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_a), 0})),
                saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_b), 0})),
            };
        });
}

TEST_P(RuntimeParamTest, EnumeratesArithmeticProgressionsEndingAtFive) {
    static constexpr size_t kInitialVarCount = 3;
    /*
     * Intent: synthesize start A and step D for a 3-term AP ending at 5.
     * Goals: add(A,D,S2), add(S2,D,five). Expected: (1,2) and (3,1).
     */
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv2, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});
    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    const expr* rv5 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), rv4, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {zero, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv6})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv6})}});

    const expr* five = peano_saved(5);
    constexpr uint32_t idx_a = 0;
    constexpr uint32_t idx_d = 1;
    constexpr uint32_t idx_s2 = 2;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_a)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_d)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("lt"), {zero, saved_expr_pool_.make_var(idx_d)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("add"), {
        saved_expr_pool_.make_var(idx_a),
        saved_expr_pool_.make_var(idx_d),
        saved_expr_pool_.make_var(idx_s2),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("add"), {
        saved_expr_pool_.make_var(idx_s2),
        saved_expr_pool_.make_var(idx_d),
        five,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kPeanoBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        {{peano_saved(1), peano_saved(2)}, {peano_saved(3), peano_saved(1)}},
        [&]() -> solution {
            return {
                saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_a), 0})),
                saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_d), 0})),
            };
        });
}

TEST_P(RuntimeParamTest, FindsGcdOfSixAndFourViaSubtraction) {
    static constexpr size_t kInitialVarCount = 1;
    /*
     * Intent: Euclidean GCD via repeated subtraction on Peano numerals.
     * sub(A,zero,A). sub(suc(A),suc(B),R):-sub(A,B,R). gcd via repeated subtraction.
     * Goal: gcd(six, four, G). Expected: G = two.
     */
    static constexpr size_t kPeanoBudget = 512;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv2, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});
    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    const expr* rv5 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), rv4, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {zero, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv6})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv6})}});
    const expr* rv7 = saved_expr_pool_.make_var(0);
    const expr* rv8 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv7}), saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv8})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("lt"), {rv7, rv8})}});

    const expr* rv9 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("sub"), {rv9, zero, rv9}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv9})}});
    const expr* rv10 = saved_expr_pool_.make_var(0);
    const expr* rv11 = saved_expr_pool_.make_var(1);
    const expr* rv12 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("sub"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv10}), saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv11}), rv12}),
        {saved_expr_pool_.make_functor(holder_.functors.id("sub"), {rv10, rv11, rv12})}});

    const expr* rv13 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("gcd"), {rv13, zero, rv13}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv13})}});
    const expr* rv14 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("gcd"), {rv14, rv14, rv14}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv14})}});
    const expr* rv15 = saved_expr_pool_.make_var(0);
    const expr* rv16 = saved_expr_pool_.make_var(1);
    const expr* rv17 = saved_expr_pool_.make_var(2);
    const expr* rv18 = saved_expr_pool_.make_var(3);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("gcd"), {rv15, rv16, rv18}),
        {saved_expr_pool_.make_functor(holder_.functors.id("lt"), {rv16, rv15}),
            saved_expr_pool_.make_functor(holder_.functors.id("sub"), {rv15, rv16, rv17}),
            saved_expr_pool_.make_functor(holder_.functors.id("gcd"), {rv17, rv16, rv18})}});
    const expr* rv19 = saved_expr_pool_.make_var(0);
    const expr* rv20 = saved_expr_pool_.make_var(1);
    const expr* rv21 = saved_expr_pool_.make_var(2);
    const expr* rv22 = saved_expr_pool_.make_var(3);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("gcd"), {rv19, rv20, rv22}),
        {saved_expr_pool_.make_functor(holder_.functors.id("lt"), {rv19, rv20}),
            saved_expr_pool_.make_functor(holder_.functors.id("sub"), {rv20, rv19, rv21}),
            saved_expr_pool_.make_functor(holder_.functors.id("gcd"), {rv19, rv21, rv22})}});

    const expr* six = peano_saved(6);
    const expr* four = peano_saved(4);
    constexpr uint32_t idx_g = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {four}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {six}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("gcd"), {
        six,
        four,
        saved_expr_pool_.make_var(idx_g),
    }));

    runtime_ref& session = make_session(kInitialVarCount, kPeanoBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        {{peano_saved(2)}},
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_g), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesTwoSubsetsOfFourElements) {
    static constexpr size_t kInitialVarCount = 2;
    /*
     * Intent: choose 2 distinct elements from {a,b,c,d} with lex ordering X before Y.
     * Goal: pair(X, Y). Expected: all 6 two-subsets.
     */
    const expr* a = saved_expr_pool_.make_functor(holder_.functors.id("a"), {});
    const expr* b = saved_expr_pool_.make_functor(holder_.functors.id("b"), {});
    const expr* c = saved_expr_pool_.make_functor(holder_.functors.id("c"), {});
    const expr* d = saved_expr_pool_.make_functor(holder_.functors.id("d"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("member"), {a}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("member"), {b}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("member"), {c}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("member"), {d}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("before"), {a, b}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("before"), {a, c}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("before"), {a, d}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("before"), {b, c}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("before"), {b, d}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("before"), {c, d}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    const expr* rv2 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("pair"), {rv1, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("member"), {rv1}),
            saved_expr_pool_.make_functor(holder_.functors.id("member"), {rv2}),
            saved_expr_pool_.make_functor(holder_.functors.id("before"), {rv1, rv2})}});

    constexpr uint32_t idx_x = 0;
    constexpr uint32_t idx_y = 1;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("pair"), {
        saved_expr_pool_.make_var(idx_x),
        saved_expr_pool_.make_var(idx_y),
    }));

    runtime_ref& session = make_session(kInitialVarCount);
    enumerate_all_solutions(
        session,
        saved_printer_,
        {{a, b}, {a, c}, {a, d}, {b, c}, {b, d}, {c, d}},
        [&]() -> solution {
            return {
                saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_x), 0})),
                saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_y), 0})),
            };
        });
}

TEST_P(RuntimeParamTest, EnumeratesBalancedGrammarStringOfLengthFour) {
    static constexpr size_t kInitialVarCount = 1;
    /*
     * Intent: grammar S -> aSb | nil; synthesize derivation of length 4 (aabb).
     * Goal: der(T), length(T, four).
     */
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    const expr* a = saved_expr_pool_.make_functor(holder_.functors.id("a"), {});
    const expr* b = saved_expr_pool_.make_functor(holder_.functors.id("b"), {});
    const expr* nil = saved_expr_pool_.make_functor(holder_.functors.id("nil"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("der"), {nil}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("len"), {nil, zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    const expr* rv2 = saved_expr_pool_.make_var(1);
    const expr* rv3 = saved_expr_pool_.make_var(2);
    const expr* wrapped = saved_expr_pool_.make_functor(holder_.functors.id("cons"), {a, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {b, rv1})});
    const expr* one = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {zero});
    const expr* two = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {one});
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("der"), {wrapped}),
        {saved_expr_pool_.make_functor(holder_.functors.id("der"), {rv1})}});
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("len"), {wrapped, rv3}),
        {saved_expr_pool_.make_functor(holder_.functors.id("len"), {rv1, rv2}),
            saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv2, two, rv3})}});

    const expr* rv4 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv4, rv4}),
        {}});
    const expr* rv5 = saved_expr_pool_.make_var(0);
    const expr* rv6 = saved_expr_pool_.make_var(1);
    const expr* rv7 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5}), rv6, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv7})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv5, rv6, rv7})}});

    const expr* aabb = saved_expr_pool_.make_functor(holder_.functors.id("cons"), {
        a, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {b, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {a, saved_expr_pool_.make_functor(k_cons_functor_id, {b, nil})})})});
    constexpr uint32_t idx_t = 0;
    const expr* four = peano_saved(4);
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("der"), {saved_expr_pool_.make_var(idx_t)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("len"), {
        saved_expr_pool_.make_var(idx_t),
        four,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kPeanoBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        {{aabb}},
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_t), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesDepthTwoTermsOverTwoConstants) {
    static constexpr size_t kInitialVarCount = 1;
    /*
     * Intent: free algebra term(F,X) with depth exactly 2 over constants {a,b}.
     * term(app(F,X)):-term(F),term(X), depth(F,zero), depth(X,zero).
     * Goal: term(T), depth(T, two). Expected 4 terms app(F,X).
     */
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    const expr* a = saved_expr_pool_.make_functor(holder_.functors.id("a"), {});
    const expr* b = saved_expr_pool_.make_functor(holder_.functors.id("b"), {});
    const expr* one = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {zero});
    const expr* two = peano_saved(2);
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("term"), {a}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("term"), {b}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("depth"), {a, zero}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("depth"), {b, zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    const expr* rv2 = saved_expr_pool_.make_var(1);
    const expr* rv3 = saved_expr_pool_.make_var(2);
    const expr* apped = saved_expr_pool_.make_functor(holder_.functors.id("app"), {rv1, rv2});
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("term"), {apped}),
        {saved_expr_pool_.make_functor(holder_.functors.id("term"), {rv1}),
            saved_expr_pool_.make_functor(holder_.functors.id("term"), {rv2}),
            saved_expr_pool_.make_functor(holder_.functors.id("depth"), {rv1, zero}),
            saved_expr_pool_.make_functor(holder_.functors.id("depth"), {rv2, zero})}});
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("depth"), {apped, two}),
        {saved_expr_pool_.make_functor(holder_.functors.id("depth"), {rv1, zero}),
            saved_expr_pool_.make_functor(holder_.functors.id("depth"), {rv2, zero})}});

    constexpr uint32_t idx_t = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("term"), {saved_expr_pool_.make_var(idx_t)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("depth"), {
        saved_expr_pool_.make_var(idx_t),
        two,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kPeanoBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        {
            {saved_expr_pool_.make_functor(holder_.functors.id("app"), {a, a})},
            {saved_expr_pool_.make_functor(holder_.functors.id("app"), {a, b})},
            {saved_expr_pool_.make_functor(holder_.functors.id("app"), {b, a})},
            {saved_expr_pool_.make_functor(holder_.functors.id("app"), {b, b})},
        },
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_t), 0}))};
        });
}

TEST_P(RuntimeParamTest, FindsEvenParityListOfLengthFour) {
    static constexpr size_t kInitialVarCount = 1;
    /*
     * Intent: mutual-recursion parity lists — evenlist(nil), oddlist(cons(T)):-evenlist(T),
     * evenlist(cons(T)):-oddlist(T). Goal: evenlist(L), len(L, four).
     */
    static constexpr size_t kPeanoBudget = 128;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    const expr* a = saved_expr_pool_.make_functor(holder_.functors.id("a"), {});
    const expr* nil = saved_expr_pool_.make_functor(holder_.functors.id("nil"), {});
    const expr* one = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {zero});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("evenlist"), {nil}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("len"), {nil, zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    const expr* rv2 = saved_expr_pool_.make_var(1);
    const expr* rv3 = saved_expr_pool_.make_var(2);
    const expr* consed = saved_expr_pool_.make_functor(holder_.functors.id("cons"), {a, rv1});
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("oddlist"), {consed}),
        {saved_expr_pool_.make_functor(holder_.functors.id("evenlist"), {rv1})}});
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("evenlist"), {consed}),
        {saved_expr_pool_.make_functor(holder_.functors.id("oddlist"), {rv1})}});
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("len"), {consed, rv3}),
        {saved_expr_pool_.make_functor(holder_.functors.id("len"), {rv1, rv2}),
            saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv2, one, rv3})}});

    const expr* rv4 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv4, rv4}),
        {}});
    const expr* rv5 = saved_expr_pool_.make_var(0);
    const expr* rv6 = saved_expr_pool_.make_var(1);
    const expr* rv7 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5}), rv6, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv7})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv5, rv6, rv7})}});

    const expr* list4 = saved_expr_pool_.make_functor(holder_.functors.id("cons"), {
        a, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {
            a, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {
                a, saved_expr_pool_.make_functor(k_cons_functor_id, {a, nil})})})});
    constexpr uint32_t idx_l = 0;
    const expr* four = peano_saved(4);
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("evenlist"), {saved_expr_pool_.make_var(idx_l)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("len"), {
        saved_expr_pool_.make_var(idx_l),
        four,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kPeanoBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        {{list4}},
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_l), 0}))};
        });
}

// Tier I — recursive + large-search CHC synthesis

TEST_P(RuntimeParamTest, EnumeratesPeanoTriplesInsideTetrahedron) {
    static constexpr size_t kInitialVarCount = 5;
    /*
     * Intent: 3D sum simplex — all (x,y,z) with x+y+z < 9.
     * Goals: add(X,Y,S), add(S,Z,T), lt(T,nine). Expected C(11,3) = 165 triples.
     * Budget: 512 (165 models, triple add chain).
     */
    static constexpr size_t kTierIBudget = 512;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv2, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});
    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    const expr* rv5 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), rv4, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {zero, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv6})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv6})}});
    const expr* rv7 = saved_expr_pool_.make_var(0);
    const expr* rv8 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv7}), saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv8})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("lt"), {rv7, rv8})}});

    std::set<solution> expected;
    for (int x = 0; x < 9; ++x) {
        for (int y = 0; y < 9 - x; ++y) {
            for (int z = 0; z < 9 - x - y; ++z)
                expected.insert({peano_saved(x), peano_saved(y), peano_saved(z)});
        }
    }
    ASSERT_EQ(expected.size(), 165u);

    constexpr uint32_t idx_x = 0;
    constexpr uint32_t idx_y = 1;
    constexpr uint32_t idx_z = 2;
    constexpr uint32_t idx_s = 3;
    constexpr uint32_t idx_t = 4;
    const expr* nine = peano_saved(9);
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_x)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_y)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_z)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("add"), {
        saved_expr_pool_.make_var(idx_x),
        saved_expr_pool_.make_var(idx_y),
        saved_expr_pool_.make_var(idx_s),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("add"), {
        saved_expr_pool_.make_var(idx_s),
        saved_expr_pool_.make_var(idx_z),
        saved_expr_pool_.make_var(idx_t),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("lt"), {
        saved_expr_pool_.make_var(idx_t),
        nine,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kTierIBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {
                saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_x), 0})),
                saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_y), 0})),
                saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_z), 0})),
            };
        });
}

TEST_P(RuntimeParamTest, EnumeratesLatticePathsFourByFour) {
    static constexpr size_t kInitialVarCount = 0;
    /*
     * Intent: monotonic lattice paths (0,0) to (4,4) via right/up — C(8,4) = 70 paths.
     * at(zero,zero). at(suc(X),Y):-at(X,Y). at(X,suc(Y)):-at(X,Y). Goal: at(four,four).
     * Harness: count solved ticks (path-list synthesis capped at 35 distinct terms here).
     * Budget: 512.
     */
    static constexpr size_t kTierIBudget = 512;
    static constexpr size_t kExpectedPaths = 70u;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("at"), {zero, zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    const expr* rv2 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("at"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1}), rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("at"), {rv1, rv2})}});
    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("at"), {rv3, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv4})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("at"), {rv3, rv4})}});

    const expr* four = peano_saved(4);
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("at"), {four, four}));

    runtime_ref& session = make_session(kInitialVarCount, kTierIBudget);
    size_t count = 0;
    while (session.next()) {
        if (session.solved())
            ++count;
    }
    EXPECT_EQ(count, kExpectedPaths);
}

TEST_P(RuntimeParamTest, EnumeratesOrderedCompositionsOfEight) {
    /*
     * Intent: ordered compositions of 8 into positive parts — 2^(8-1) = 128 lists.
     * compose(zero,nil). compose(S,cons(K,Rest)):-add(K,RestSum,S),compose(RestSum,Rest) for K=1..8.
     * Goal: compose(eight,L). Budget: 4096 (127 distinct at 2048 — one composition missing).
     */
    static constexpr size_t kTierIBudget = 4096;
    static constexpr size_t kCompositionInitialVarCount = 1;
    static constexpr int kCompositionSum = 8;
    static constexpr size_t kExpectedCompositions = 128u;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    const expr* one = peano_saved(1);
    const expr* two = peano_saved(2);
    const expr* three = peano_saved(3);
    const expr* four = peano_saved(4);
    const expr* five = peano_saved(5);
    const expr* six = peano_saved(6);
    const expr* seven = peano_saved(7);
    const expr* eight = peano_saved(kCompositionSum);
    const expr* nil = saved_expr_pool_.make_functor(holder_.functors.id("nil"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("compose"), {zero, nil}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv2, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});
    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    const expr* rv5 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), rv4, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make_var(0);
    const expr* rv7 = saved_expr_pool_.make_var(1);
    const expr* rv8 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv6, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {one, rv8})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {one, rv7, rv6}),
            saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv7, rv8})}});
    const expr* rv9 = saved_expr_pool_.make_var(0);
    const expr* rv10 = saved_expr_pool_.make_var(1);
    const expr* rv11 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv9, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {two, rv11})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {two, rv10, rv9}),
            saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv10, rv11})}});
    const expr* rv12 = saved_expr_pool_.make_var(0);
    const expr* rv13 = saved_expr_pool_.make_var(1);
    const expr* rv14 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv12, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {three, rv14})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {three, rv13, rv12}),
            saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv13, rv14})}});
    const expr* rv15 = saved_expr_pool_.make_var(0);
    const expr* rv16 = saved_expr_pool_.make_var(1);
    const expr* rv17 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv15, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {four, rv17})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {four, rv16, rv15}),
            saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv16, rv17})}});
    const expr* rv18 = saved_expr_pool_.make_var(0);
    const expr* rv19 = saved_expr_pool_.make_var(1);
    const expr* rv20 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv18, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {five, rv20})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {five, rv19, rv18}),
            saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv19, rv20})}});
    const expr* rv21 = saved_expr_pool_.make_var(0);
    const expr* rv22 = saved_expr_pool_.make_var(1);
    const expr* rv23 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv21, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {six, rv23})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {six, rv22, rv21}),
            saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv22, rv23})}});
    const expr* rv24 = saved_expr_pool_.make_var(0);
    const expr* rv25 = saved_expr_pool_.make_var(1);
    const expr* rv26 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv24, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {seven, rv26})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {seven, rv25, rv24}),
            saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv25, rv26})}});
    const expr* rv27 = saved_expr_pool_.make_var(0);
    const expr* rv28 = saved_expr_pool_.make_var(1);
    const expr* rv29 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv27, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {eight, rv29})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {eight, rv28, rv27}),
            saved_expr_pool_.make_functor(holder_.functors.id("compose"), {rv28, rv29})}});

    std::set<solution> expected;
    const expr* parts[] = {nullptr, one, two, three, four, five, six, seven, eight};
    const auto build_compositions = [&](const auto& self, int remaining, const expr* list) -> void {
        if (remaining == 0) {
            expected.insert({list});
            return;
        }
        for (int part = 1; part <= remaining; ++part)
            self(self, remaining - part, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {parts[part], list}));
    };
    build_compositions(build_compositions, kCompositionSum, nil);
    ASSERT_EQ(expected.size(), kExpectedCompositions);

    constexpr uint32_t idx_l = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("compose"), {eight, saved_expr_pool_.make_var(idx_l)}));

    runtime_ref& session = make_session(kCompositionInitialVarCount, kTierIBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_l), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesBinaryStringsNoConsecutiveOnesLengthTen) {
    /*
     * Intent: binary strings length 10 with no adjacent 1s — Fibonacci F(12) = 144.
     * good(nil).
     * good(cons(b0,T)):-good(T).
     * good(cons(b1,nil)).
     * good(cons(b1,cons(b0,T))):-good(T).
     * MSB-first cons: three good heads (0 anywhere; 1 only at end or before 0).
     * len(nil,zero). len(cons(_,T),suc(L)):-len(T,L). Goal: good(S),len(S,ten).
     * Budget: 1024 (144 models, automaton depth 10).
     */
    static constexpr size_t kTierIBudget = 1024;
    static constexpr int kStringLength = 10;
    static constexpr size_t kExpectedStrings = 144u;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    const expr* nil = saved_expr_pool_.make_functor(holder_.functors.id("nil"), {});
    const expr* b0 = saved_expr_pool_.make_functor(holder_.functors.id("b0"), {});
    const expr* b1 = saved_expr_pool_.make_functor(holder_.functors.id("b1"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("good"), {nil}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("len"), {nil, zero}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("good"), {saved_expr_pool_.make_functor(holder_.functors.id("cons"), {b0, rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("good"), {rv1})}});
    const expr* rv2 = saved_expr_pool_.make_var(0);
    const expr* rv3 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("good"), {saved_expr_pool_.make_functor(holder_.functors.id("cons"), {b1, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {b0, rv2})})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("good"), {rv2})}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("good"), {saved_expr_pool_.make_functor(holder_.functors.id("cons"), {b1, nil})}), {}});

    const expr* rv4 = saved_expr_pool_.make_var(0);
    const expr* rv5 = saved_expr_pool_.make_var(1);
    const expr* rv6 = saved_expr_pool_.make_var(2);
    const expr* rv7 = saved_expr_pool_.make_var(3);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("len"), {saved_expr_pool_.make_functor(holder_.functors.id("cons"), {rv4, rv5}), saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv7})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("len"), {rv5, rv7})}});

    const expr* ten = peano_saved(kStringLength);
    constexpr uint32_t idx_s = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("good"), {saved_expr_pool_.make_var(idx_s)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("len"), {
        saved_expr_pool_.make_var(idx_s),
        ten,
    }));

    std::set<solution> expected;
    const auto build_strings = [&](const auto& self, int remaining, bool prev_one, const expr* list) -> void {
        if (remaining == 0) {
            expected.insert({list});
            return;
        }
        self(self, remaining - 1, false, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {b0, list}));
        if (!prev_one)
            self(self, remaining - 1, true, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {b1, list}));
    };
    build_strings(build_strings, kStringLength, false, nil);
    ASSERT_EQ(expected.size(), kExpectedStrings);

    constexpr size_t kBinaryStringsInitialVarCount = 1;
    
    runtime_ref& session = make_session(kBinaryStringsInitialVarCount, kTierIBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_s), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesStaircasePathsOneOrTwoSummingToTen) {
    /*
     * Intent: climb 10 stairs with steps 1 or 2 — Fibonacci F(11) = 89 ordered step lists.
     * steps(zero,nil). steps(S,cons(one,Rest)):-add(one,RestSum,S),steps(RestSum,Rest).
     * steps(S,cons(two,Rest)):-add(two,RestSum,S),steps(RestSum,Rest). Goal: steps(ten,P).
     * Budget: 1024 (89 models, dual branching per level).
     */
    static constexpr size_t kTierIBudget = 1024;
    static constexpr int kStaircaseHeight = 10;
    static constexpr size_t kExpectedPaths = 89u;
    static constexpr size_t kStaircaseInitialVarCount = 1;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    const expr* one = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {zero});
    const expr* two = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {one});
    const expr* nil = saved_expr_pool_.make_functor(holder_.functors.id("nil"), {});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("steps"), {zero, nil}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv2, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});
    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    const expr* rv5 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), rv4, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make_var(0);
    const expr* rv7 = saved_expr_pool_.make_var(1);
    const expr* rv8 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("steps"), {rv6, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {one, rv8})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {one, rv7, rv6}),
            saved_expr_pool_.make_functor(holder_.functors.id("steps"), {rv7, rv8})}});
    const expr* rv9 = saved_expr_pool_.make_var(0);
    const expr* rv10 = saved_expr_pool_.make_var(1);
    const expr* rv11 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("steps"), {rv9, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {two, rv11})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {two, rv10, rv9}),
            saved_expr_pool_.make_functor(holder_.functors.id("steps"), {rv10, rv11})}});

    const expr* ten = peano_saved(kStaircaseHeight);
    constexpr uint32_t idx_p = 0;
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("steps"), {ten, saved_expr_pool_.make_var(idx_p)}));

    std::set<solution> expected;
    const auto build_paths = [&](const auto& self, int remaining, const expr* list) -> void {
        if (remaining == 0) {
            expected.insert({list});
            return;
        }
        if (remaining >= 1)
            self(self, remaining - 1, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {one, list}));
        if (remaining >= 2)
            self(self, remaining - 2, saved_expr_pool_.make_functor(holder_.functors.id("cons"), {two, list}));
    };
    build_paths(build_paths, kStaircaseHeight, nil);
    ASSERT_EQ(expected.size(), kExpectedPaths);

    runtime_ref& session = make_session(kStaircaseInitialVarCount, kTierIBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_p), 0}))};
        });
}

TEST_P(RuntimeParamTest, EnumeratesFibIndexPairsWithSumBelowThirty) {
    static constexpr size_t kInitialVarCount = 5;
    /*
     * Intent: ordered (I,J) with fib(I)+fib(J) < 3 — 2D recursive fib + add coupling.
     * Goals: nat(I),nat(J),fib(I,VI),fib(J,VJ),add(VI,VJ,S),lt(S,three). Indices 0..3.
     * Expected: 11 pairs. Budget: 256 (search cost is highly seed-sensitive).
     */
    static constexpr size_t kTierIBudget = 256;
    static constexpr int kFibBruteForceBound = 3;
    static constexpr int kSumBound = 3;

    auto peano_saved = [&](int n) -> const expr* {
        const expr* p = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
        for (int i = 0; i < n; ++i)
            p = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {p});
        return p;
    };

    const expr* zero = saved_expr_pool_.make_functor(holder_.functors.id("zero"), {});
    const expr* one = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {zero});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("nat"), {zero}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("fib"), {zero, zero}), {}});
    database.push(rule{saved_expr_pool_.make_functor(holder_.functors.id("fib"), {one, one}), {}});

    const expr* rv1 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv1})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv1})}});

    const expr* rv2 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {zero, rv2, rv2}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv2})}});
    const expr* rv3 = saved_expr_pool_.make_var(0);
    const expr* rv4 = saved_expr_pool_.make_var(1);
    const expr* rv5 = saved_expr_pool_.make_var(2);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("add"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv3}), rv4, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv5})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv3, rv4, rv5})}});

    const expr* rv6 = saved_expr_pool_.make_var(0);
    const expr* rv7 = saved_expr_pool_.make_var(1);
    const expr* rv8 = saved_expr_pool_.make_var(2);
    const expr* rv9 = saved_expr_pool_.make_var(3);
    const expr* suc_rv6 = saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv6});
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("fib"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {suc_rv6}), rv9}),
        {saved_expr_pool_.make_functor(holder_.functors.id("fib"), {rv6, rv7}),
            saved_expr_pool_.make_functor(holder_.functors.id("fib"), {suc_rv6, rv8}),
            saved_expr_pool_.make_functor(holder_.functors.id("add"), {rv7, rv8, rv9})}});

    const expr* rv10 = saved_expr_pool_.make_var(0);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {zero, saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv10})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("nat"), {rv10})}});
    const expr* rv11 = saved_expr_pool_.make_var(0);
    const expr* rv12 = saved_expr_pool_.make_var(1);
    database.push(rule{
        saved_expr_pool_.make_functor(holder_.functors.id("lt"), {saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv11}), saved_expr_pool_.make_functor(holder_.functors.id("suc"), {rv12})}),
        {saved_expr_pool_.make_functor(holder_.functors.id("lt"), {rv11, rv12})}});

    std::set<solution> expected;
    const auto fib_at = [&](int n) -> int {
        if (n <= 1)
            return n;
        int a = 0;
        int b = 1;
        for (int k = 2; k <= n; ++k) {
            const int c = a + b;
            a = b;
            b = c;
        }
        return b;
    };
    for (int i = 0; i <= kFibBruteForceBound; ++i) {
        for (int j = 0; j <= kFibBruteForceBound; ++j) {
            if (fib_at(i) + fib_at(j) < kSumBound)
                expected.insert({peano_saved(i), peano_saved(j)});
        }
    }
    ASSERT_EQ(expected.size(), 11u);

    constexpr uint32_t idx_i = 0;
    constexpr uint32_t idx_j = 1;
    constexpr uint32_t idx_vi = 2;
    constexpr uint32_t idx_vj = 3;
    constexpr uint32_t idx_s = 4;
    const expr* sum_limit = peano_saved(kSumBound);
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_i)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("nat"), {saved_expr_pool_.make_var(idx_j)}));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("fib"), {
        saved_expr_pool_.make_var(idx_i),
        saved_expr_pool_.make_var(idx_vi),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("fib"), {
        saved_expr_pool_.make_var(idx_j),
        saved_expr_pool_.make_var(idx_vj),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("add"), {
        saved_expr_pool_.make_var(idx_vi),
        saved_expr_pool_.make_var(idx_vj),
        saved_expr_pool_.make_var(idx_s),
    }));
    initial_goals.push(saved_expr_pool_.make_functor(holder_.functors.id("lt"), {
        saved_expr_pool_.make_var(idx_s),
        sum_limit,
    }));

    runtime_ref& session = make_session(kInitialVarCount, kTierIBudget);
    enumerate_all_solutions(
        session,
        saved_printer_,
        expected,
        [&]() -> solution {
            return {
                saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_i), 0})),
                saved_expr_pool_.import(session.normalize({saved_expr_pool_.make_var(idx_j), 0})),
            };
        });
}
INSTANTIATE_TEST_SUITE_P(
    AllRuntimes,
    RuntimeParamTest,
    ::testing::Values(runtime_kind::basic, runtime_kind::ridge, runtime_kind::horizon),
    [](const ::testing::TestParamInfo<runtime_kind>& info) {
        switch (info.param) {
            case runtime_kind::basic: return "basic";
            case runtime_kind::ridge: return "ridge";
            case runtime_kind::horizon: return "horizon";
        }
        return "unknown";
    });
