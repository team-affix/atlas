#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_term_stack.hpp"

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

    const expr* deep(uint32_t depth, const expr* base) {
        const expr* term = base;
        for (uint32_t level = 0; level < depth; ++level)
            term = functor("s", {term});
        return term;
    }

    // f(t, t) nested `depth` times: linear as a DAG, exponential as a tree.
    const expr* shared(uint32_t depth, const expr* base) {
        const expr* term = base;
        for (uint32_t level = 0; level < depth; ++level)
            term = functor("f", {term, term});
        return term;
    }

    bool unify(const expr* lhs, const expr* rhs) {
        return stack.unifier.unify(framed_expr{lhs, 0}, framed_expr{rhs, 0}).has_value();
    }

    void bind(uint32_t index, const expr* value) {
        stack.instantiate(index, framed_expr{value, 0});
    }

    const expr* normalize(const expr* skeleton) {
        return stack.normalizer.normalize(framed_expr{skeleton, 0});
    }

    const expr* reference_normalize(const expr* skeleton) {
        if (const expr::var* variable = std::get_if<expr::var>(&skeleton->content)) {
            const ubs_class variable_class = stack.bind_map.find(variable->index);
            if (!variable_class.value.has_value())
                return var(variable_class.rep);
            skeleton = variable_class.value->skeleton;
        }
        const expr::functor& term = std::get<expr::functor>(skeleton->content);
        std::vector<const expr*> args;
        for (const expr* arg : term.args)
            args.push_back(reference_normalize(arg));
        return stack.pool.make_functor(term.id, args);
    }

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
    bind(0, deep(k_deep, var(1)));
    bind(1, atom("z"));
    EXPECT_EQ(normalize(var(0)), deep(k_deep, atom("z")));
}

TEST_F(PudDeepTermsIntegrationTest, NormalizerHandlesAVeryDeepChainOfBindings) {
    const uint32_t chain = 50000;
    for (uint32_t index = 0; index < chain; ++index)
        bind(index, functor("s", {var(index + 1)}));
    EXPECT_EQ(normalize(var(0)), deep(chain, var(chain)));
}

TEST_F(PudDeepTermsIntegrationTest, ExporterHandlesAVeryDeepTerm) {
    std::unordered_map<uint32_t, uint32_t> translations;
    auto exporter = stack.terms.make_exporter(stack.bind_map, 10, translations);
    EXPECT_EQ(exporter.export_expr(framed_expr{deep(k_deep, var(3)), 10}), deep(k_deep, var(10)));
    EXPECT_EQ(translations.size(), 1u);
}

TEST_F(PudDeepTermsIntegrationTest, ExporterFollowsAVeryDeepChainOfCalleeBindings) {
    const uint32_t chain = 50000;
    for (uint32_t index = 0; index < chain; ++index)
        bind(10 + index, functor("s", {var(11 + index)}));
    std::unordered_map<uint32_t, uint32_t> translations;
    auto exporter = stack.terms.make_exporter(stack.bind_map, 10, translations);
    EXPECT_EQ(exporter.export_expr(framed_expr{var(0), 10}), deep(chain, var(10)));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierUnifiesTwoVeryDeepTerms) {
    EXPECT_TRUE(unify(deep(k_deep, var(0)), deep(k_deep, atom("z"))));
    EXPECT_EQ(normalize(var(0)), atom("z"));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierFailsAtTheBottomOfTwoVeryDeepTerms) {
    EXPECT_FALSE(unify(deep(k_deep, atom("a")), deep(k_deep, atom("b"))));
}

TEST_F(PudDeepTermsIntegrationTest, OccursCheckFindsAVariableAtTheBottomOfAVeryDeepTerm) {
    EXPECT_FALSE(unify(var(0), deep(k_deep, var(0))));
}

TEST_F(PudDeepTermsIntegrationTest, OccursCheckPassesAVeryDeepAcyclicTerm) {
    EXPECT_TRUE(unify(var(0), deep(k_deep, var(1))));
    EXPECT_EQ(normalize(var(0)), deep(k_deep, var(1)));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierUnifiesAVeryWideTerm) {
    std::vector<const expr*> holes;
    std::vector<const expr*> fills;
    for (uint32_t index = 0; index < 20000; ++index) {
        holes.push_back(var(index));
        fills.push_back(atom(index % 2 == 0 ? "a" : "b"));
    }
    EXPECT_TRUE(unify(functor("w", holes), functor("w", fills)));
    EXPECT_EQ(normalize(var(19999)), atom("b"));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierBindsArgumentsLeftToRight) {
    EXPECT_FALSE(unify(functor("p", {var(0), var(0)}), functor("p", {atom("a"), atom("b")})));
    EXPECT_EQ(normalize(var(0)), atom("a"));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierUnifiesASharedGroundTermWithItselfInLinearTime) {
    const expr* term = shared(k_shared_depth, atom("z"));
    EXPECT_TRUE(stack.unifier.unify(framed_expr{term, 0}, framed_expr{term, 0}));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierUnifiesASharedGroundTermAcrossFramesInLinearTime) {
    const expr* term = shared(k_shared_depth, atom("z"));
    EXPECT_TRUE(stack.unifier.unify(framed_expr{term, 0}, framed_expr{term, 7}));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierUnifiesTwoDistinctSharedTermsInLinearTime) {
    EXPECT_TRUE(unify(shared(k_shared_depth, var(0)), shared(k_shared_depth, atom("z"))));
    EXPECT_EQ(normalize(var(0)), atom("z"));
}

TEST_F(PudDeepTermsIntegrationTest, OccursCheckSearchesASharedValueInLinearTime) {
    EXPECT_TRUE(unify(var(0), shared(k_shared_depth, var(1))));
    EXPECT_FALSE(unify(var(1), functor("g", {var(0)})));
}

TEST_F(PudDeepTermsIntegrationTest, NormalizerResolvesSharedBindingsInLinearTime) {
    for (uint32_t index = 0; index < k_shared_depth; ++index)
        bind(index, functor("f", {var(index + 1), var(index + 1)}));
    bind(k_shared_depth, atom("z"));
    EXPECT_EQ(normalize(var(0)), shared(k_shared_depth, atom("z")));
}

TEST_F(PudDeepTermsIntegrationTest, ExporterRebuildsASharedTermInLinearTime) {
    std::unordered_map<uint32_t, uint32_t> translations;
    auto exporter = stack.terms.make_exporter(stack.bind_map, 10, translations);
    EXPECT_EQ(exporter.export_expr(framed_expr{shared(k_shared_depth, var(2)), 10}), shared(k_shared_depth, var(10)));
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
            bind(index, value);
        }
        const expr* term = random_term(rng, 6, 0, 9);
        EXPECT_EQ(normalize(term), reference_normalize(term));
    }
}
