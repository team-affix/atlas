#include <random>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_node_added_substitutions.hpp"
#include "infrastructure/pud_normalizer_factory.hpp"
#include "infrastructure/pud_structure_watch_descent.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "infrastructure/pud_translating_normalizer.hpp"
#include "infrastructure/pud_unifier_factory.hpp"

namespace {

constexpr uint32_t k_deep = 200000;

struct PudDeepTermsIntegrationTest : public ::testing::Test {
    using bind_map_t = ubs_transient_bind_map<expr_pool, expr_pool>;
    using normalizer_factory_t = pud_normalizer_factory<globalizer, expr_pool, bind_map_t>;
    using normalizer_t = normalizer_factory_t::normalizer_t;
    using unifier_factory_t = pud_unifier_factory<globalizer, normalizer_t, bind_map_t>;
    using translating_normalizer_t = pud_translating_normalizer<globalizer, expr_pool, expr_pool, bind_map_t>;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    normalizer_factory_t normalizer_factory{globalize, exprs};
    unifier_factory_t unifier_factory{globalize};
    pud_transient_ubs_state transient_ubs_state;
    bind_map_t bind_map = transient_ubs_state.make_bind_map(exprs, exprs);
    normalizer_t normalizer = normalizer_factory.make(bind_map);

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), args);
    }
    const expr* atom(const char* name) { return functor(name, {}); }

    const expr* deep(uint32_t depth, const expr* base) {
        const expr* term = base;
        for (uint32_t level = 0; level < depth; ++level)
            term = functor("s", {term});
        return term;
    }

    bool unify(const expr* lhs, const expr* rhs) {
        auto unifier = unifier_factory.make(normalizer, bind_map);
        return unifier.unify(framed_expr{lhs, 0}, framed_expr{rhs, 0});
    }

    void bind(uint32_t index, const expr* value) {
        const expr::functor& value_functor = std::get<expr::functor>(value->content);
        bind_map.instantiate(index, value_functor.id, value_functor.args);
    }

    // A straightforward recursive normalization, kept only as an oracle on shallow terms.
    const expr* reference_normalize(bind_map_t& bindings, const expr* skeleton) {
        if (const expr::var* variable = std::get_if<expr::var>(&skeleton->content)) {
            const expr* value = bindings.resolve(variable->index);
            if (std::holds_alternative<expr::var>(value->content))
                return value;
            skeleton = value;
        }
        const expr::functor& term = std::get<expr::functor>(skeleton->content);
        std::vector<const expr*> args;
        for (const expr* arg : term.args)
            args.push_back(reference_normalize(bindings, arg));
        return exprs.make_functor(term.id, args);
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

}  // namespace

TEST_F(PudDeepTermsIntegrationTest, GlobalNormalizerHandlesAVeryDeepTerm) {
    bind(0, deep(k_deep, var(1)));
    bind(1, atom("z"));
    EXPECT_EQ(normalizer.normalize(framed_expr{var(0), 0}), deep(k_deep, atom("z")));
}

TEST_F(PudDeepTermsIntegrationTest, GlobalNormalizerHandlesAVeryDeepChainOfBindings) {
    const uint32_t chain = 50000;
    for (uint32_t index = 0; index < chain; ++index)
        bind(index, functor("s", {var(index + 1)}));
    EXPECT_EQ(normalizer.normalize(framed_expr{var(0), 0}), deep(chain, var(chain)));
}

TEST_F(PudDeepTermsIntegrationTest, TranslatingNormalizerHandlesAVeryDeepTerm) {
    translating_normalizer_t translating{globalize, exprs, exprs, bind_map};
    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(
        translating.normalize(framed_expr{deep(k_deep, var(3)), 10}, 10, translation),
        deep(k_deep, var(10)));
    EXPECT_EQ(translation.size(), 1u);
}

TEST_F(PudDeepTermsIntegrationTest, UnifierUnifiesTwoVeryDeepTerms) {
    EXPECT_TRUE(unify(deep(k_deep, var(0)), deep(k_deep, atom("z"))));
    EXPECT_EQ(normalizer.normalize(framed_expr{var(0), 0}), atom("z"));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierFailsAtTheBottomOfTwoVeryDeepTerms) {
    EXPECT_FALSE(unify(deep(k_deep, atom("a")), deep(k_deep, atom("b"))));
}

TEST_F(PudDeepTermsIntegrationTest, OccursCheckFindsAVariableAtTheBottomOfAVeryDeepTerm) {
    EXPECT_FALSE(unify(var(0), deep(k_deep, var(0))));
}

TEST_F(PudDeepTermsIntegrationTest, OccursCheckPassesAVeryDeepAcyclicTerm) {
    EXPECT_TRUE(unify(var(0), deep(k_deep, var(1))));
    EXPECT_EQ(normalizer.normalize(framed_expr{var(0), 0}), deep(k_deep, var(1)));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierUnifiesAVeryWideTerm) {
    std::vector<const expr*> holes;
    std::vector<const expr*> fills;
    for (uint32_t index = 0; index < 20000; ++index) {
        holes.push_back(var(index));
        fills.push_back(atom(index % 2 == 0 ? "a" : "b"));
    }
    EXPECT_TRUE(unify(functor("w", holes), functor("w", fills)));
    EXPECT_EQ(normalizer.normalize(framed_expr{var(19999), 0}), atom("b"));
}

TEST_F(PudDeepTermsIntegrationTest, UnifierBindsArgumentsLeftToRight) {
    EXPECT_FALSE(unify(functor("p", {var(0), var(0)}), functor("p", {atom("a"), atom("b")})));
    EXPECT_EQ(normalizer.normalize(framed_expr{var(0), 0}), atom("a"));
}

TEST_F(PudDeepTermsIntegrationTest, StructureWatchWatchesAVeryDeepGoal) {
    pud_node_added_substitutions substitutions;
    substitutions.store(1, {pud_substitution{.var_idx = 0, .value = atom("a")}});
    pud_structure_watch_descent<pud_node_added_substitutions> watcher{
        substitutions, {functor("p", {deep(k_deep, var(0))})}};
    const auto watched = watcher.descend(1, {}, {deep(k_deep, var(0))});
    EXPECT_EQ(watched.structure_changes, (std::vector<pud_watch_head_id>{0}));
    EXPECT_EQ(watched.added_head_ids, (std::vector<pud_watch_head_id>{1}));
}

TEST_F(PudDeepTermsIntegrationTest, NormalizerMatchesARecursiveReferenceOnRandomTerms) {
    for (unsigned seed = 0; seed < 300; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        std::mt19937 rng{seed};
        pud_transient_ubs_state seed_state;
        bind_map_t seed_bind_map = seed_state.make_bind_map(exprs, exprs);
        normalizer_t seed_normalizer = normalizer_factory.make(seed_bind_map);
        // Each variable is bound only to terms over higher variables, so the bindings are acyclic.
        for (uint32_t index = 0; index < 8; ++index) {
            if (rng() % 2 == 0)
                continue;
            const expr* value = random_term(rng, 3, index + 1, 8);
            if (std::holds_alternative<expr::var>(value->content))
                continue;
            const expr::functor& value_functor = std::get<expr::functor>(value->content);
            seed_bind_map.instantiate(index, value_functor.id, value_functor.args);
        }
        const expr* term = random_term(rng, 6, 0, 9);
        EXPECT_EQ(seed_normalizer.normalize(framed_expr{term, 0}), reference_normalize(seed_bind_map, term));
    }
}
