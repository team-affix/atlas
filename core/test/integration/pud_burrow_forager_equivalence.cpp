#include <cstddef>
#include <cstdint>
#include <deque>
#include <tuple>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_burrow_runtime.hpp"
#include "infrastructure/pud_forager_runtime.hpp"
#include "infrastructure/pud_module.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/rule.hpp"

// Burrow works node states out from their paths when read and drops them when its cache is full; Forager keeps them
// from activation on. A worked-out state equals a kept one, so the two make the same unfolds whatever the cache budget.
// Both run the same program side by side, over their own copies of its modules and one shared expression pool, and
// must report the same step, solution and depth every time, then stop for the same reason after the same number of
// unfolds. A budget of zero drops every state after every step.
struct PudBurrowForagerEquivalenceIntegrationTest : public ::testing::TestWithParam<std::tuple<size_t, size_t>> {
    static constexpr pud_module_id k_library = 1;
    static constexpr pud_module_id k_query = 2;
    static constexpr size_t k_max_unfolds = 600;
    static constexpr size_t k_max_node_depth = 64;
    static constexpr size_t k_max_open_goals = 64;

    test_functors functors;
    expr_pool exprs;
    std::vector<rule> library;
    std::vector<rule> query;
    std::deque<pud_module> burrow_modules;
    std::deque<pud_module> forager_modules;
    size_t compared_steps = 0;
    size_t solutions = 0;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
    const expr* peano(size_t value) {
        const expr* number = atom("z");
        for (size_t step = 0; step < value; ++step)
            number = functor("s", {number});
        return number;
    }

    void load(std::deque<pud_module>& modules, pud_burrow_runtime* burrow, pud_forager_runtime* forager) {
        modules.emplace_back(k_library, std::vector<pud_module_id>{}, library);
        modules.emplace_back(k_query, std::vector<pud_module_id>{k_library}, query);
        for (pud_module& module : modules) {
            install_shared_functor_names(module, functors);
            if (burrow != nullptr)
                burrow->register_module(module);
            if (forager != nullptr)
                forager->register_module(module);
        }
    }

    void run_side_by_side() {
        const pud_global_node_id focus{.module = k_query, .node = 0};
        const size_t cascade_depth = std::get<0>(GetParam());
        const size_t state_cache_bytes = std::get<1>(GetParam());
        pud_burrow_runtime burrow(
            exprs, focus, k_max_unfolds, cascade_depth, k_max_node_depth, k_max_open_goals, state_cache_bytes, false);
        pud_forager_runtime forager(
            exprs, focus, k_max_unfolds, cascade_depth, k_max_node_depth, k_max_open_goals, false);
        load(burrow_modules, &burrow, nullptr);
        load(forager_modules, nullptr, &forager);
        for (;;) {
            const bool burrow_stepped = burrow.next();
            ASSERT_EQ(burrow_stepped, forager.next()) << "step " << compared_steps;
            if (!burrow_stepped)
                break;
            ASSERT_EQ(burrow.last_step(), forager.last_step()) << "step " << compared_steps;
            ASSERT_EQ(burrow.solution(), forager.solution()) << "step " << compared_steps;
            if (burrow.solution() != nullptr) {
                ASSERT_EQ(burrow.solution_depth(), forager.solution_depth()) << "step " << compared_steps;
                ++solutions;
            }
            ++compared_steps;
        }
        EXPECT_EQ(burrow.stop_reason(), forager.stop_reason());
        EXPECT_EQ(burrow.unfold_count(), forager.unfold_count());
    }
};

TEST_P(PudBurrowForagerEquivalenceIntegrationTest, PeanoAdditionPairsSummingToSix) {
    library = {
        rule(functor("add", {atom("z"), var(0), var(0)}), {}, 1),
        rule(
            functor("add", {functor("s", {var(0)}), var(1), functor("s", {var(2)})}),
            {functor("add", {var(0), var(1), var(2)})},
            3),
    };
    query = {rule(functor("q", {var(0), var(1)}), {functor("add", {var(0), var(1), peano(6)})}, 2)};
    run_side_by_side();
    EXPECT_GT(solutions, 0u);
}

TEST_P(PudBurrowForagerEquivalenceIntegrationTest, PeanoProductsOfFour) {
    library = {
        rule(functor("nat", {atom("z")}), {}, 0),
        rule(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1),
        rule(functor("add", {atom("z"), var(0), var(0)}), {}, 1),
        rule(
            functor("add", {functor("s", {var(0)}), var(1), functor("s", {var(2)})}),
            {functor("add", {var(0), var(1), var(2)})},
            3),
        rule(functor("mul", {atom("z"), var(0), atom("z")}), {}, 1),
        rule(
            functor("mul", {functor("s", {var(0)}), var(1), var(2)}),
            {functor("mul", {var(0), var(1), var(3)}), functor("add", {var(3), var(1), var(2)})},
            4),
    };
    query = {rule(
        functor("q", {var(0), var(1)}),
        {functor("nat", {var(0)}), functor("nat", {var(1)}), functor("mul", {var(0), var(1), peano(4)})},
        2)};
    run_side_by_side();
    EXPECT_GT(compared_steps, 10u);
}

TEST_P(PudBurrowForagerEquivalenceIntegrationTest, GraphColoringsOfATriangleWithATail) {
    library = {};
    for (const char* first : {"red", "green", "blue"})
        for (const char* second : {"red", "green", "blue"})
            if (first != second)
                library.push_back(rule(functor("differ", {atom(first), atom(second)}), {}, 0));
    query = {rule(
        functor("q", {var(0), var(1), var(2), var(3)}),
        {functor("differ", {var(0), var(1)}), functor("differ", {var(1), var(2)}), functor("differ", {var(0), var(2)}),
         functor("differ", {var(2), var(3)})},
        4)};
    run_side_by_side();
    EXPECT_GT(solutions, 0u);
}

TEST_P(PudBurrowForagerEquivalenceIntegrationTest, ReachabilityThroughARecursivePath) {
    library = {
        rule(functor("edge", {atom("a"), atom("b")}), {}, 0),
        rule(functor("edge", {atom("b"), atom("c")}), {}, 0),
        rule(functor("edge", {atom("c"), atom("a")}), {}, 0),
        rule(functor("edge", {atom("c"), atom("d")}), {}, 0),
        rule(functor("path", {var(0), var(1)}), {functor("edge", {var(0), var(1)})}, 2),
        rule(
            functor("path", {var(0), var(2)}),
            {functor("edge", {var(0), var(1)}), functor("path", {var(1), var(2)})},
            3),
    };
    query = {rule(
        functor("q", {var(0)}), {functor("path", {atom("a"), var(0)}), functor("path", {var(0), atom("d")})}, 1)};
    run_side_by_side();
    EXPECT_GT(solutions, 0u);
}

INSTANTIATE_TEST_SUITE_P(
    CascadeDepthsAndBudgets,
    PudBurrowForagerEquivalenceIntegrationTest,
    ::testing::Combine(
        ::testing::Values(size_t{0}, size_t{1}, size_t{2}),
        ::testing::Values(size_t{0}, size_t{4096}, size_t{1} << 30)));
