#include <deque>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_lodestar_runtime.hpp"
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
    pud_lodestar_runtime runtime(exprs, 2, 32, 10);
    for (pud_module& module : modules)
        runtime.register_module(module);
    ASSERT_TRUE(runtime.next());
    ASSERT_NE(runtime.solution(), nullptr);
    EXPECT_EQ(runtime.solution_depth(), 0u);
    EXPECT_EQ(runtime.solution(), atom("lodestar_query"));
    EXPECT_FALSE(runtime.next());
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
    pud_lodestar_runtime runtime(exprs, 2, 32, 10);
    for (pud_module& module : modules)
        runtime.register_module(module);
    ASSERT_TRUE(runtime.next());
    ASSERT_NE(runtime.solution(), nullptr);
    EXPECT_GE(runtime.solution_depth(), 1u);
    const expr::functor& head = std::get<expr::functor>(runtime.solution()->content);
    ASSERT_EQ(head.args.size(), 1u);
    EXPECT_EQ(head.args[0], abc);
    EXPECT_FALSE(runtime.next());
}

TEST_F(PudLodestarManifestTest, GoalWithNoCandidatesRefutes) {
    std::deque<pud_module> modules;
    modules.emplace_back(1, std::vector<pud_module_id>{}, std::vector<rule>{});
    modules.emplace_back(2, std::vector<pud_module_id>{1}, std::vector<rule>{
        rule(atom("lodestar_query"), {atom("missing")}, 0),
    });
    pud_lodestar_runtime runtime(exprs, 2, 32, 10);
    for (pud_module& module : modules)
        runtime.register_module(module);
    ASSERT_TRUE(runtime.next());
    EXPECT_EQ(runtime.solution(), nullptr);
    EXPECT_FALSE(runtime.next());
}
