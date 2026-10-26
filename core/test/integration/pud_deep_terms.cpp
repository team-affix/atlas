#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_term_stack.hpp"
#include "reference_pud_normalize.hpp"

namespace {

constexpr uint32_t k_deep = 200000;
constexpr uint32_t k_shared_depth = 64;

struct PudDeepTermsIntegrationTest : public ::testing::Test {
    test_functors functors;
    pud_term_stack stack;

    const expr* var(uint32_t index) { return stack.pool.make_var(index); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return stack.pool.make_functor(functors.id(name), args);
    }
    const expr* atom(const char* name) { return functor(name, {}); }

    const expr* random_term(std::mt19937& rng, uint32_t depth, uint32_t lowest_var, uint32_t var_count) {
        const bool leaf = depth == 0 || rng() % 3 == 0;
        if (leaf) {
            if (rng() % 2 == 0)
                return atom(rng() % 2 == 0 ? "a" : "b");
            return var(lowest_var + rng() % var_count);
        }
        std::vector<const expr*> args;
        const uint32_t arity = 1 + rng() % 3;
        for (uint32_t arg = 0; arg < arity; ++arg)
            args.push_back(random_term(rng, depth - 1, lowest_var, var_count));
        return functor(rng() % 2 == 0 ? "f" : "g", args);
    }
};

}

TEST_F(PudDeepTermsIntegrationTest, NormalizerHandlesAVeryDeepTerm) {
    const expr* deep_bound = var(1);
    for (uint32_t level = 0; level < k_deep; ++level)
        deep_bound = functor("s", {deep_bound});
    stack.bind_map.instantiate(stack.bind_map.find(0u), framed_functor_from_framed_expr(framed_expr{deep_bound, 0u}));
    stack.bind_map.instantiate(stack.bind_map.find(1u), framed_functor_from_framed_expr(framed_expr{atom("z"), 0u}));
    const expr* deep_expected = atom("z");
    for (uint32_t level = 0; level < k_deep; ++level)
        deep_expected = functor("s", {deep_expected});
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{var(0), 0u}), deep_expected);
}

TEST_F(PudDeepTermsIntegrationTest, NormalizerHandlesAVeryDeepChainOfBindings) {
    const uint32_t chain = 50000;
    for (uint32_t index = 0; index < chain; ++index)
        stack.bind_map.instantiate(stack.bind_map.find(index), framed_functor_from_framed_expr(framed_expr{functor("s", {var(index + 1)}), 0u}));
    const expr* deep_chain = var(chain);
    for (uint32_t level = 0; level < chain; ++level)
        deep_chain = functor("s", {deep_chain});
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{var(0), 0u}), deep_chain);
}

TEST_F(PudDeepTermsIntegrationTest, ExporterHandlesAVeryDeepTerm) {
    const expr* deep_term = var(3);
    for (uint32_t level = 0; level < k_deep; ++level)
        deep_term = functor("s", {deep_term});
    const expr* deep_exported = var(10);
    for (uint32_t level = 0; level < k_deep; ++level)
        deep_exported = functor("s", {deep_exported});
    std::unordered_map<uint32_t, uint32_t> translations;
    auto exporter = stack.terms.make_exporter(stack.bind_map, 10, translations);
    EXPECT_EQ(exporter.export_expr(framed_expr{deep_term, 10u}), deep_exported);
    EXPECT_EQ(translations.size(), 1u);
}

TEST_F(PudDeepTermsIntegrationTest, ExporterFollowsAVeryDeepChainOfCalleeBindings) {
    const uint32_t chain = 50000;
    for (uint32_t index = 0; index < chain; ++index)
        stack.bind_map.instantiate(stack.bind_map.find(10 + index), framed_functor_from_framed_expr(framed_expr{functor("s", {var(11 + index)}), 0u}));
    const expr* deep_chain = var(10);
    for (uint32_t level = 0; level < chain; ++level)
        deep_chain = functor("s", {deep_chain});
    std::unordered_map<uint32_t, uint32_t> translations;
    auto exporter = stack.terms.make_exporter(stack.bind_map, 10, translations);
    EXPECT_EQ(exporter.export_expr(framed_expr{var(0), 10u}), deep_chain);
}

TEST_F(PudDeepTermsIntegrationTest, UnifierUnifiesTwoVeryDeepTerms) {
    const expr* deep_lhs = var(0);
    for (uint32_t level = 0; level < k_deep; ++level)
        deep_lhs = functor("s", {deep_lhs});
    const expr* deep_rhs = atom("z");
    for (uint32_t level = 0; level < k_deep; ++level)
        deep_rhs = functor("s", {deep_rhs});
    EXPECT_TRUE(stack.unifier.unify(framed_expr{deep_lhs, 0u}, framed_expr{deep_rhs, 0u}).has_value());
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{var(0), 0u}), atom("z"));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierFailsAtTheBottomOfTwoVeryDeepTerms) {
    const expr* deep_a = atom("a");
    for (uint32_t level = 0; level < k_deep; ++level)
        deep_a = functor("s", {deep_a});
    const expr* deep_b = atom("b");
    for (uint32_t level = 0; level < k_deep; ++level)
        deep_b = functor("s", {deep_b});
    EXPECT_FALSE(stack.unifier.unify(framed_expr{deep_a, 0u}, framed_expr{deep_b, 0u}).has_value());
}

TEST_F(PudDeepTermsIntegrationTest, OccursCheckFindsAVariableAtTheBottomOfAVeryDeepTerm) {
    const expr* deep_var = var(0);
    for (uint32_t level = 0; level < k_deep; ++level)
        deep_var = functor("s", {deep_var});
    EXPECT_FALSE(stack.unifier.unify(framed_expr{var(0), 0u}, framed_expr{deep_var, 0u}).has_value());
}

TEST_F(PudDeepTermsIntegrationTest, OccursCheckPassesAVeryDeepAcyclicTerm) {
    const expr* deep_var = var(1);
    for (uint32_t level = 0; level < k_deep; ++level)
        deep_var = functor("s", {deep_var});
    EXPECT_TRUE(stack.unifier.unify(framed_expr{var(0), 0u}, framed_expr{deep_var, 0u}).has_value());
    const expr* deep_expected = var(1);
    for (uint32_t level = 0; level < k_deep; ++level)
        deep_expected = functor("s", {deep_expected});
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{var(0), 0u}), deep_expected);
}

TEST_F(PudDeepTermsIntegrationTest, UnifierUnifiesAVeryWideTerm) {
    std::vector<const expr*> holes;
    std::vector<const expr*> fills;
    for (uint32_t index = 0; index < 20000; ++index) {
        holes.push_back(var(index));
        fills.push_back(atom(index % 2 == 0 ? "a" : "b"));
    }
    EXPECT_TRUE(stack.unifier.unify(
        framed_expr{functor("w", holes), 0u},
        framed_expr{functor("w", fills), 0u}).has_value());
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{var(19999), 0u}), atom("b"));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierBindsArgumentsLeftToRight) {
    EXPECT_FALSE(stack.unifier.unify(
        framed_expr{functor("p", {var(0), var(0)}), 0u},
        framed_expr{functor("p", {atom("a"), atom("b")}), 0u}).has_value());
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{var(0), 0u}), atom("a"));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierUnifiesASharedGroundTermWithItselfInLinearTime) {
    const expr* term = atom("z");
    for (uint32_t level = 0; level < k_shared_depth; ++level)
        term = functor("f", {term, term});
    EXPECT_TRUE(stack.unifier.unify(framed_expr{term, 0u}, framed_expr{term, 0u}));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierUnifiesASharedGroundTermAcrossFramesInLinearTime) {
    const expr* term = atom("z");
    for (uint32_t level = 0; level < k_shared_depth; ++level)
        term = functor("f", {term, term});
    EXPECT_TRUE(stack.unifier.unify(framed_expr{term, 0u}, framed_expr{term, 7u}));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierUnifiesTwoDistinctSharedTermsInLinearTime) {
    const expr* shared_lhs = var(0);
    for (uint32_t level = 0; level < k_shared_depth; ++level)
        shared_lhs = functor("f", {shared_lhs, shared_lhs});
    const expr* shared_rhs = atom("z");
    for (uint32_t level = 0; level < k_shared_depth; ++level)
        shared_rhs = functor("f", {shared_rhs, shared_rhs});
    EXPECT_TRUE(stack.unifier.unify(
        framed_expr{shared_lhs, 0u}, framed_expr{shared_rhs, 0u}).has_value());
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{var(0), 0u}), atom("z"));
}

TEST_F(PudDeepTermsIntegrationTest, OccursCheckSearchesASharedValueInLinearTime) {
    const expr* shared_var = var(1);
    for (uint32_t level = 0; level < k_shared_depth; ++level)
        shared_var = functor("f", {shared_var, shared_var});
    EXPECT_TRUE(stack.unifier.unify(framed_expr{var(0), 0u}, framed_expr{shared_var, 0u}).has_value());
    EXPECT_FALSE(stack.unifier.unify(
        framed_expr{var(1), 0u},
        framed_expr{functor("g", {var(0)}), 0u}).has_value());
}

TEST_F(PudDeepTermsIntegrationTest, NormalizerResolvesSharedBindingsInLinearTime) {
    for (uint32_t index = 0; index < k_shared_depth; ++index)
        stack.bind_map.instantiate(stack.bind_map.find(index), framed_functor_from_framed_expr(framed_expr{functor("f", {var(index + 1), var(index + 1)}), 0u}));
    stack.bind_map.instantiate(stack.bind_map.find(k_shared_depth), framed_functor_from_framed_expr(framed_expr{atom("z"), 0u}));
    const expr* shared_expected = atom("z");
    for (uint32_t level = 0; level < k_shared_depth; ++level)
        shared_expected = functor("f", {shared_expected, shared_expected});
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{var(0), 0u}), shared_expected);
}

TEST_F(PudDeepTermsIntegrationTest, ExporterRebuildsASharedTermInLinearTime) {
    const expr* shared_term = var(2);
    for (uint32_t level = 0; level < k_shared_depth; ++level)
        shared_term = functor("f", {shared_term, shared_term});
    const expr* shared_exported = var(10);
    for (uint32_t level = 0; level < k_shared_depth; ++level)
        shared_exported = functor("f", {shared_exported, shared_exported});
    std::unordered_map<uint32_t, uint32_t> translations;
    auto exporter = stack.terms.make_exporter(stack.bind_map, 10, translations);
    EXPECT_EQ(exporter.export_expr(framed_expr{shared_term, 10u}), shared_exported);
}

TEST_F(PudDeepTermsIntegrationTest, NormalizerMatchesARecursiveReferenceOnRandomTerms) {
    for (unsigned seed = 0; seed < 300; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        std::mt19937 rng{seed};
        stack.bind_map = ubs_bind_map{pud_ubs_state{}};
        for (uint32_t index = 0; index < 8; ++index) {
            if (rng() % 2 == 0)
                continue;
            const expr* value = random_term(rng, 3, index + 1, 8);
            if (std::holds_alternative<expr::var>(value->content))
                continue;
            stack.bind_map.instantiate(stack.bind_map.find(index), framed_functor_from_framed_expr(framed_expr{value, 0u}));
        }
        const expr* term = random_term(rng, 6, 0, 9);
        EXPECT_EQ(
            stack.normalizer.normalize(framed_expr{term, 0u}),
            reference_pud_normalize(stack, term));
    }
}
