#include <algorithm>
#include <deque>
#include <set>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_lodestar_runtime.hpp"
#include "pud_lodestar_steps.hpp"
#include "infrastructure/pud_module.hpp"
#include "value_objects/rule.hpp"

namespace {

struct PudLodestarManifestTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
};

}

TEST_F(PudLodestarManifestTest, EmptyConjectureFactIsDepthZero) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{});
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(atom("lodestar_query"), {}, 0),
    });
    pud_lodestar_runtime runtime(exprs, pud_global_node_id{.module = 2, .node = 0}, 32, 10, 64, 64, false);
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        runtime.register_module(module);
    }
    ASSERT_TRUE(advance_to_report(runtime));
    ASSERT_NE(runtime.solution(), nullptr);
    EXPECT_EQ(runtime.solution_depth(), 0u);
    EXPECT_EQ(runtime.solution(), atom("lodestar_query"));
    EXPECT_FALSE(advance_to_report(runtime));
}

TEST_F(PudLodestarManifestTest, FactInDependencyNormalizesTheConjectureHead) {
    const expr* abc = atom("abc");
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
        rule(functor("f", {abc}), {}, 0),
    });
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(functor("lodestar_query", {exprs.make_var(0)}), {functor("f", {exprs.make_var(0)})}, 1),
    });
    pud_lodestar_runtime runtime(exprs, pud_global_node_id{.module = 2, .node = 0}, 32, 10, 64, 64, false);
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        runtime.register_module(module);
    }
    ASSERT_TRUE(advance_to_report(runtime));
    ASSERT_NE(runtime.solution(), nullptr);
    EXPECT_GE(runtime.solution_depth(), 1u);
    const expr::functor& head = std::get<expr::functor>(runtime.solution()->content);
    ASSERT_EQ(head.args.size(), 1u);
    EXPECT_EQ(head.args[0], abc);
    EXPECT_FALSE(advance_to_report(runtime));
}

TEST_F(PudLodestarManifestTest, GoalWithNoCandidatesRefutes) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{});
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(atom("lodestar_query"), {atom("missing")}, 0),
    });
    pud_lodestar_runtime runtime(exprs, pud_global_node_id{.module = 2, .node = 0}, 32, 10, 64, 64, false);
    for (pud_module& module : modules) {
        install_shared_functor_names(module, this->functors);
        runtime.register_module(module);
    }
    ASSERT_TRUE(advance_to_report(runtime));
    EXPECT_EQ(runtime.solution(), nullptr);
    EXPECT_FALSE(advance_to_report(runtime));
    EXPECT_EQ(runtime.stop_reason(), pud_search_stop_reason::refuted);
}

// q(X) :- nat(X) has a solution at every depth, so only a session limit can end the search.
struct PudLodestarSessionLimitTest : public PudLodestarManifestTest {
    std::deque<pud_module> modules;

    void SetUp() override {
        const expr* zero = atom("zero");
        modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(functor("nat", {zero}), {}, 0),
            rule(functor("nat", {functor("s", {exprs.make_var(0)})}), {functor("nat", {exprs.make_var(0)})}, 1),
        });
        modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
            rule(functor("q", {exprs.make_var(0)}), {functor("nat", {exprs.make_var(0)})}, 1),
        });
    }

    void register_into(pud_lodestar_runtime& runtime) {
        for (pud_module& module : modules) {
            install_shared_functor_names(module, this->functors);
            runtime.register_module(module);
        }
    }
};

TEST_F(PudLodestarSessionLimitTest, ADepthLimitedSearchEndsExhaustedAndLeavesTheConjectureUnrefuted) {
    pud_lodestar_runtime runtime(exprs, pud_global_node_id{.module = 2, .node = 0}, 1000, 10, 2, 64, false);
    register_into(runtime);
    size_t ticks = 0;
    while (advance_to_report(runtime))
        ++ticks;
    EXPECT_GT(ticks, 0u);
    EXPECT_EQ(runtime.stop_reason(), pud_search_stop_reason::exhausted);
    EXPECT_FALSE(modules[1].is_refuted(0));
}

TEST_F(PudLodestarSessionLimitTest, ASpentBudgetEndsTheSessionWithoutRefutingIt) {
    pud_lodestar_runtime runtime(exprs, pud_global_node_id{.module = 2, .node = 0}, 1, 10, 64, 64, false);
    register_into(runtime);
    while (advance_to_report(runtime)) {
    }
    EXPECT_EQ(runtime.stop_reason(), pud_search_stop_reason::budget_spent);
    EXPECT_FALSE(modules[1].is_refuted(0));
}

TEST_F(PudLodestarSessionLimitTest, ASessionRunsUntilItStops) {
    pud_lodestar_runtime runtime(exprs, pud_global_node_id{.module = 2, .node = 0}, 1000, 10, 64, 64, false);
    EXPECT_EQ(runtime.stop_reason(), pud_search_stop_reason::running);
}

// Leaves a depth limit blocked are never written as refuted, so a later session over the same modules with a
// deeper limit picks them up again and keeps unfolding.
TEST_F(PudLodestarSessionLimitTest, ADeeperSessionOverTheSameModulesResumesFromTheBlockedLeaves) {
    size_t shallow_solutions = 0;
    {
        pud_lodestar_runtime shallow(exprs, pud_global_node_id{.module = 2, .node = 0}, 1000, 10, 2, 64, false);
        register_into(shallow);
        while (advance_to_report(shallow))
            shallow_solutions += shallow.solution() != nullptr ? 1 : 0;
        ASSERT_EQ(shallow.stop_reason(), pud_search_stop_reason::exhausted);
    }
    size_t refuted_nodes = 0;
    for (pud_node_id node = 0; node < modules[1].count_nodes(); ++node)
        refuted_nodes += modules[1].is_refuted(node) ? 1 : 0;
    EXPECT_EQ(refuted_nodes, 0u);
    EXPECT_GT(shallow_solutions, 0u);
    const size_t stored_nodes = modules[1].count_nodes();
    pud_lodestar_runtime deeper(exprs, pud_global_node_id{.module = 2, .node = 0}, 30, 10, 64, 64, false);
    for (pud_module& module : modules)
        deeper.register_module(module);
    size_t deepest = 0;
    while (advance_to_report(deeper)) {
        if (deeper.solution() == nullptr)
            continue;
        deepest = std::max(deepest, deeper.solution_depth());
    }
    EXPECT_GT(modules[1].count_nodes(), stored_nodes);
    EXPECT_GT(deepest, 2u);
}

// A session over modules an earlier session already unfolded reports only the solutions it derives
// itself, unless it is asked to report the stored ones again.
TEST_F(PudLodestarSessionLimitTest, AResumedSessionReportsOnlyNewSolutionsUnlessAskedForStoredOnes) {
    std::set<const expr*> first_run;
    {
        pud_lodestar_runtime first(exprs, pud_global_node_id{.module = 2, .node = 0}, 30, 10, 64, 64, false);
        register_into(first);
        while (advance_to_report(first)) {
            if (first.solution() != nullptr)
                first_run.insert(first.solution());
        }
    }
    ASSERT_FALSE(first_run.empty());
    std::set<const expr*> resumed_run;
    {
        pud_lodestar_runtime resumed(exprs, pud_global_node_id{.module = 2, .node = 0}, 30, 10, 64, 64, false);
        for (pud_module& module : modules)
            resumed.register_module(module);
        while (advance_to_report(resumed)) {
            if (resumed.solution() != nullptr)
                resumed_run.insert(resumed.solution());
        }
    }
    EXPECT_FALSE(resumed_run.empty());
    for (const expr* solution : resumed_run)
        EXPECT_FALSE(first_run.contains(solution));
    std::set<const expr*> reported_again;
    pud_lodestar_runtime again(exprs, pud_global_node_id{.module = 2, .node = 0}, 0, 10, 64, 64, true);
    for (pud_module& module : modules)
        again.register_module(module);
    while (advance_to_report(again)) {
        if (again.solution() != nullptr)
            reported_again.insert(again.solution());
    }
    for (const expr* solution : first_run)
        EXPECT_TRUE(reported_again.contains(solution));
}
