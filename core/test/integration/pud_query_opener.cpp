#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_axioms.hpp"
#include "infrastructure/pud_node_added_substitutions.hpp"
#include "infrastructure/pud_node_heads.hpp"
#include "infrastructure/pud_normalizer_factory.hpp"
#include "infrastructure/pud_query_opener.hpp"
#include "infrastructure/pud_replay_descent.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "infrastructure/pud_unifier_factory.hpp"

struct PudQueryOpenerIntegrationTest : public ::testing::Test {
    using bind_map_t = ubs_bind_map<
        expr_pool, expr_pool, pud_transient_ubs_state, pud_transient_ubs_state, pud_transient_ubs_state,
        pud_transient_ubs_state, pud_transient_ubs_state, pud_transient_ubs_state>;
    using normalizer_factory_t = pud_normalizer_factory<globalizer, expr_pool, bind_map_t>;
    using normalizer_t = normalizer_factory_t::normalizer_t;
    using unifier_factory_t = pud_unifier_factory<globalizer, normalizer_t, bind_map_t>;
    using unifier_t = unifier_factory_t::unifier_t;
    using replay_descent_t = pud_replay_descent<
        bind_map_t, normalizer_t, unifier_t, expr_pool, expr_pool, normalizer_factory_t,
        unifier_factory_t, pud_node_added_substitutions>;
    using opener_t = pud_query_opener<
        replay_descent_t, bind_map_t, normalizer_t, unifier_t, pud_axioms, pud_node_heads,
        expr_pool, expr_pool, normalizer_factory_t, unifier_factory_t, pud_node_added_substitutions>;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    pud_axioms axioms;
    pud_node_heads heads;
    pud_node_added_substitutions substitutions;
    normalizer_factory_t normalizer_factory{globalize, exprs};
    unifier_factory_t unifier_factory{globalize};
    opener_t opener{axioms, heads, exprs, exprs, normalizer_factory, unifier_factory, substitutions};
    pud_ubs_state empty_state;
    pud_node_id next_node = 1;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), args);
    }
    const expr* atom(const char* name) { return functor(name, {}); }

    pud_node_id axiom(const expr* head) {
        const pud_node_id node = next_node++;
        axioms.mark_axiom(node);
        heads.store(node, head);
        return node;
    }

    const expr* resolve(const pud_ubs_state& state, uint32_t index) {
        pud_transient_ubs_state transient_ubs_state{state};
        auto bind_map = transient_ubs_state.make_bind_map(exprs, exprs);
        auto normalizer = normalizer_factory.make(bind_map);
        return normalizer.normalize(framed_expr{var(index), 0});
    }

    std::vector<pud_node_id> nodes_of(const std::vector<replay_descent_t>& opened) {
        std::vector<pud_node_id> nodes;
        for (const replay_descent_t& root_descent : opened)
            nodes.push_back(root_descent.node());
        return nodes;
    }
};

TEST_F(PudQueryOpenerIntegrationTest, OnlyAxiomsWhoseHeadsResolveTheGoalAreOpened) {
    const pud_node_id red = axiom(functor("color", {atom("red")}));
    const pud_node_id blue = axiom(functor("color", {atom("blue")}));
    axiom(functor("warm", {atom("red")}));
    EXPECT_THAT(nodes_of(opener.open(empty_state, functor("color", {var(0)}), 1)), ::testing::ElementsAre(red, blue));
}

TEST_F(PudQueryOpenerIntegrationTest, HeadedNodesThatAreNotAxiomsAreNeverOpened) {
    const pud_node_id red = axiom(functor("color", {atom("red")}));
    heads.store(next_node++, functor("color", {atom("blue")}));
    EXPECT_THAT(nodes_of(opener.open(empty_state, functor("color", {var(0)}), 1)), ::testing::ElementsAre(red));
}

TEST_F(PudQueryOpenerIntegrationTest, EachOpenedReplayHoldsItsResolution) {
    axiom(functor("color", {atom("red")}));
    axiom(functor("color", {atom("blue")}));
    const auto opened = opener.open(empty_state, functor("color", {var(0)}), 1);
    ASSERT_EQ(opened.size(), 2u);
    EXPECT_EQ(resolve(opened.at(0).ubs_state(), 0), atom("red"));
    EXPECT_EQ(resolve(opened.at(1).ubs_state(), 0), atom("blue"));
}

TEST_F(PudQueryOpenerIntegrationTest, CallersBindingsAreRespected) {
    axiom(functor("color", {atom("red")}));
    const pud_node_id blue = axiom(functor("color", {atom("blue")}));
    pud_transient_ubs_state transient_ubs_state{empty_state};
    auto bind_map = transient_ubs_state.make_bind_map(exprs, exprs);
    bind_map.instantiate(0, functors.id("blue"), {});
    const pud_ubs_state caller = transient_ubs_state.to_persistent_state();
    EXPECT_THAT(nodes_of(opener.open(caller, functor("color", {var(0)}), 1)), ::testing::ElementsAre(blue));
}

TEST_F(PudQueryOpenerIntegrationTest, RootHeadIsLiftedToTheFrameOffset) {
    axiom(functor("nat", {functor("s", {var(0)})}));
    const auto opened = opener.open(empty_state, functor("nat", {var(0)}), 3);
    ASSERT_EQ(opened.size(), 1u);
    EXPECT_EQ(resolve(opened.at(0).ubs_state(), 0), functor("s", {var(3)}));
}

TEST_F(PudQueryOpenerIntegrationTest, NoRootResolvesTheGoal) {
    axiom(functor("color", {atom("red")}));
    EXPECT_TRUE(opener.open(empty_state, functor("warm", {var(0)}), 1).empty());
}

TEST_F(PudQueryOpenerIntegrationTest, CallersStateIsLeftUntouched) {
    axiom(functor("color", {atom("red")}));
    opener.open(empty_state, functor("color", {var(0)}), 1);
    EXPECT_EQ(resolve(empty_state, 0), var(0));
}
