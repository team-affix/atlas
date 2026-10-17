#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_binding_descent.hpp"
#include "infrastructure/pud_node_added_substitutions.hpp"
#include "infrastructure/pud_normalizer_factory.hpp"
#include "infrastructure/pud_replay_descent.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "infrastructure/pud_unifier_factory.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_substitution.hpp"

struct PudReplayDescentIntegrationTest : public ::testing::Test {
    using bind_map_t = ubs_bind_map<
        expr_pool, expr_pool, pud_transient_ubs_state, pud_transient_ubs_state, pud_transient_ubs_state,
        pud_transient_ubs_state, pud_transient_ubs_state, pud_transient_ubs_state>;
    using normalizer_factory_t = pud_normalizer_factory<globalizer, expr_pool, bind_map_t>;
    using normalizer_t = normalizer_factory_t::normalizer_t;
    using unifier_factory_t = pud_unifier_factory<globalizer, normalizer_t, bind_map_t>;
    using unifier_t = unifier_factory_t::unifier_t;
    using binding_descent_t = pud_binding_descent<bind_map_t, pud_node_added_substitutions, expr_pool, expr_pool>;
    using replay_descent_t = pud_replay_descent<
        bind_map_t,
        normalizer_t,
        unifier_t,
        expr_pool,
        expr_pool,
        normalizer_factory_t,
        unifier_factory_t,
        pud_node_added_substitutions>;

    test_functors                  functors;
    expr_pool                      exprs;
    globalizer                     globalize;
    normalizer_factory_t           normalizer_factory_{globalize, exprs};
    unifier_factory_t              unifier_factory_{globalize};
    pud_node_added_substitutions node_specs_;

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    const pud_node_id root = 0;

    std::optional<replay_descent_t> open(
        const pud_ubs_state& caller_state,
        const expr* query_expr,
        const expr* axiom_head,
        uint32_t frame_offset) {
        pud_transient_ubs_state transient{caller_state};
        bind_map_t bind_map = transient.make_bind_map(exprs, exprs);
        normalizer_t normalizer = normalizer_factory_.make(bind_map);
        unifier_t unifier = unifier_factory_.make(normalizer, bind_map);
        if (!unifier.unify(framed_expr{query_expr, 0u}, framed_expr{axiom_head, frame_offset}))
            return std::nullopt;
        return replay_descent_t{
            exprs,
            exprs,
            normalizer_factory_,
            unifier_factory_,
            node_specs_,
            root,
            transient.to_persistent_state(),
            frame_offset};
    }

    const expr* resolve_global(const replay_descent_t& descent, uint32_t global_var_index) {
        pud_transient_ubs_state transient{descent.ubs_state()};
        bind_map_t bind_map = transient.make_bind_map(exprs, exprs);
        return bind_map.resolve(global_var_index);
    }

    const expr* resolve_at_frame(const replay_descent_t& descent, uint32_t frame_offset, uint32_t local_var_index) {
        return resolve_global(descent, globalize.globalize(frame_offset, local_var_index));
    }

    void expect_unbound_global(const replay_descent_t& descent, uint32_t global_var_index) {
        const expr* resolved = resolve_global(descent, global_var_index);
        const expr::var* rep_var = std::get_if<expr::var>(&resolved->content);
        ASSERT_NE(rep_var, nullptr);
        EXPECT_EQ(rep_var->index, global_var_index);
    }
};

TEST_F(PudReplayDescentIntegrationTest, SpecsReplayLiftedToFrameOffset) {
    const pud_node_id child = 1;
    const uint32_t frame_offset = 3;
    node_specs_.store(child, {
        pud_substitution{.var_idx = 0, .value = functor("a", {})},
        pud_substitution{.var_idx = 1, .value = functor("b", {})},
    });
    const expr* head = functor("main", {var(0), var(1), var(2)});
    std::optional<replay_descent_t> opened = open({}, head, head, frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_child = opened->descend(child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(resolve_at_frame(*at_child, frame_offset, 0), functor("a", {}));
    EXPECT_EQ(resolve_at_frame(*at_child, frame_offset, 1), functor("b", {}));
    EXPECT_EQ(resolve_at_frame(*at_child, 0u, 0), functor("a", {}));
    EXPECT_EQ(resolve_at_frame(*at_child, 0u, 1), functor("b", {}));
}

TEST_F(PudReplayDescentIntegrationTest, DifferentFrameOffsetsStayIndependent) {
    const pud_node_id child = 1;
    node_specs_.store(child, {pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    const expr* head = var(0);
    std::optional<replay_descent_t> at_one = open({}, var(0), head, 1u);
    std::optional<replay_descent_t> at_five = open({}, var(0), head, 5u);
    ASSERT_TRUE(at_one.has_value());
    ASSERT_TRUE(at_five.has_value());
    std::optional<replay_descent_t> child_one = at_one->descend(child);
    std::optional<replay_descent_t> child_five = at_five->descend(child);
    ASSERT_TRUE(child_one.has_value());
    ASSERT_TRUE(child_five.has_value());
    EXPECT_EQ(resolve_global(*child_one, 1u), functor("a", {}));
    EXPECT_EQ(resolve_global(*child_five, 5u), functor("a", {}));
    expect_unbound_global(*child_one, 5u);
    expect_unbound_global(*child_five, 1u);
}

TEST_F(PudReplayDescentIntegrationTest, OpeningFlowFromBindingDescentState) {
    const pud_node_id binding_child = 11;
    const pud_node_id agree_child = 1;
    const pud_node_id contradict_child = 2;
    const uint32_t frame_offset = 1u;

    node_specs_.store(binding_child, {pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    binding_descent_t caller_binding =
        binding_descent_t{node_specs_, exprs, exprs}.descend(binding_child);

    std::optional<replay_descent_t> opened = open(caller_binding.ubs_state(), var(0), var(0), frame_offset);
    ASSERT_TRUE(opened.has_value());

    node_specs_.store(agree_child, {pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    EXPECT_TRUE(opened->descend(agree_child).has_value());

    node_specs_.store(contradict_child, {pud_substitution{.var_idx = 0, .value = functor("b", {})}});
    EXPECT_FALSE(opened->descend(contradict_child).has_value());
}

TEST_F(PudReplayDescentIntegrationTest, ContradictsInheritedCallerBindingRefutes) {
    const pud_node_id child = 1;
    node_specs_.store(child, {pud_substitution{.var_idx = 0, .value = functor("b", {})}});
    std::optional<replay_descent_t> opened = open({}, functor("a", {}), var(0), 1);
    ASSERT_TRUE(opened.has_value());
    EXPECT_FALSE(opened->descend(child).has_value());
}

TEST_F(PudReplayDescentIntegrationTest, ContradictsEarlierReplayedSpecRefutes) {
    const pud_node_id good_child = 1;
    const pud_node_id bad_child = 2;
    node_specs_.store(good_child, {pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    node_specs_.store(bad_child, {pud_substitution{.var_idx = 0, .value = functor("x", {})}});
    std::optional<replay_descent_t> opened = open({}, functor("main", {}), functor("main", {}), 0u);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_good = opened->descend(good_child);
    ASSERT_TRUE(at_good.has_value());
    EXPECT_FALSE(at_good->descend(bad_child).has_value());
}

TEST_F(PudReplayDescentIntegrationTest, ManySpecsReplay) {
    const uint32_t substitution_count = 200;
    std::vector<pud_substitution> substitutions;
    for (uint32_t substitution_index = 0; substitution_index < substitution_count; ++substitution_index) {
        std::string label = "v" + std::to_string(substitution_index);
        substitutions.push_back(pud_substitution{.var_idx = substitution_index, .value = functor(label.c_str(), {})});
    }
    const pud_node_id child = 1;
    node_specs_.store(child, std::move(substitutions));
    std::optional<replay_descent_t> opened = open({}, functor("main", {}), functor("main", {}), 0u);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_child = opened->descend(child);
    ASSERT_TRUE(at_child.has_value());
    for (uint32_t substitution_index = 0; substitution_index < substitution_count; ++substitution_index) {
        std::string label = "v" + std::to_string(substitution_index);
        EXPECT_EQ(resolve_at_frame(*at_child, 0u, substitution_index), functor(label.c_str(), {}));
    }
}

TEST_F(PudReplayDescentIntegrationTest, ForkedDescentsFromSharedPrefixStayIndependent) {
    const pud_node_id bob_child = 1;
    const pud_node_id x_child = 2;
    const uint32_t frame_offset = 3;
    node_specs_.store(bob_child, {
        pud_substitution{.var_idx = 0, .value = functor("tom", {})},
        pud_substitution{.var_idx = 1, .value = functor("bob", {})},
    });
    node_specs_.store(x_child, {
        pud_substitution{.var_idx = 0, .value = functor("tom", {})},
        pud_substitution{.var_idx = 1, .value = functor("x", {})},
    });
    const expr* head = functor("parent", {var(0), var(1)});
    const expr* query = functor("parent", {var(0), var(2)});
    std::optional<replay_descent_t> opened = open({}, query, head, frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_bob = opened->descend(bob_child);
    std::optional<replay_descent_t> at_x = opened->descend(x_child);
    ASSERT_TRUE(at_bob.has_value());
    ASSERT_TRUE(at_x.has_value());
    EXPECT_EQ(resolve_at_frame(*at_bob, frame_offset, 1), functor("bob", {}));
    EXPECT_EQ(resolve_at_frame(*at_x, frame_offset, 1), functor("x", {}));
}

TEST_F(PudReplayDescentIntegrationTest, CallerVarThroughHeadBindingSeesCalleeFunctor) {
    const pud_node_id child = 1;
    const uint32_t frame_offset = 3;
    node_specs_.store(child, {
        pud_substitution{.var_idx = 0, .value = functor("tom", {})},
        pud_substitution{.var_idx = 1, .value = functor("bob", {})},
    });
    const expr* head = functor("parent", {var(0), var(1)});
    const expr* query = functor("parent", {var(0), var(2)});
    std::optional<replay_descent_t> opened = open({}, query, head, frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_child = opened->descend(child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(resolve_at_frame(*at_child, 0u, 2), functor("bob", {}));
}

TEST_F(PudReplayDescentIntegrationTest, VarSpecEquatesCalleeVarsAtTheFrameOffset) {
    const pud_node_id child = 1;
    const uint32_t frame_offset = 3;
    node_specs_.store(child, {pud_substitution{.var_idx = 1, .value = var(0)}});
    std::optional<replay_descent_t> opened = open({}, functor("p", {}), functor("p", {}), frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_child = opened->descend(child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(resolve_at_frame(*at_child, frame_offset, 1), resolve_at_frame(*at_child, frame_offset, 0));
    expect_unbound_global(*at_child, globalize.globalize(frame_offset, 0));
    expect_unbound_global(*at_child, 0u);
    expect_unbound_global(*at_child, 1u);
}

TEST_F(PudReplayDescentIntegrationTest, SpecValueVarsAreLiftedToTheFrameOffset) {
    const pud_node_id child = 1;
    const pud_node_id grandchild = 2;
    const uint32_t frame_offset = 2;
    node_specs_.store(child, {pud_substitution{.var_idx = 0, .value = functor("f", {var(1)})}});
    node_specs_.store(grandchild, {pud_substitution{.var_idx = 1, .value = functor("b", {})}});
    std::optional<replay_descent_t> opened = open({}, functor("p", {}), functor("p", {}), frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_child = opened->descend(child);
    ASSERT_TRUE(at_child.has_value());
    expect_unbound_global(*at_child, 1u);
    std::optional<replay_descent_t> at_grandchild = at_child->descend(grandchild);
    ASSERT_TRUE(at_grandchild.has_value());
    EXPECT_EQ(resolve_at_frame(*at_grandchild, frame_offset, 1), functor("b", {}));
    expect_unbound_global(*at_grandchild, 1u);
}

TEST_F(PudReplayDescentIntegrationTest, SiblingStillDescendsAfterARefutedChild) {
    const pud_node_id bad_child = 1;
    const pud_node_id good_child = 2;
    node_specs_.store(bad_child, {pud_substitution{.var_idx = 0, .value = functor("b", {})}});
    node_specs_.store(good_child, {pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    std::optional<replay_descent_t> opened = open({}, functor("a", {}), var(0), 1);
    ASSERT_TRUE(opened.has_value());
    ASSERT_FALSE(opened->descend(bad_child).has_value());
    std::optional<replay_descent_t> at_good = opened->descend(good_child);
    ASSERT_TRUE(at_good.has_value());
    EXPECT_EQ(resolve_at_frame(*at_good, 1, 0), functor("a", {}));
}

TEST_F(PudReplayDescentIntegrationTest, NodeWithNoSpecsKeepsTheBindings) {
    const pud_node_id child = 1;
    node_specs_.store(child, {});
    std::optional<replay_descent_t> opened = open({}, functor("a", {}), var(0), 4);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_child = opened->descend(child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(resolve_at_frame(*at_child, 4, 0), functor("a", {}));
}

TEST_F(PudReplayDescentIntegrationTest, ThreeLevelReplayAccumulates) {
    const uint32_t frame_offset = 5;
    node_specs_.store(1, {pud_substitution{.var_idx = 0, .value = functor("f", {var(1)})}});
    node_specs_.store(2, {pud_substitution{.var_idx = 1, .value = functor("g", {var(2)})}});
    node_specs_.store(3, {pud_substitution{.var_idx = 2, .value = functor("c", {})}});
    std::optional<replay_descent_t> at = open({}, functor("p", {}), functor("p", {}), frame_offset);
    for (pud_node_id level_node = 1; level_node <= 3; ++level_node) {
        ASSERT_TRUE(at.has_value()) << "level " << level_node;
        at = at->descend(level_node);
    }
    ASSERT_TRUE(at.has_value());
    EXPECT_EQ(resolve_at_frame(*at, frame_offset, 2), functor("c", {}));
    const expr* resolved_one = resolve_at_frame(*at, frame_offset, 1);
    const expr::functor* g_functor = std::get_if<expr::functor>(&resolved_one->content);
    ASSERT_NE(g_functor, nullptr);
    EXPECT_EQ(g_functor->id, functors.id("g"));
}

TEST_F(PudReplayDescentIntegrationTest, RefutationDeepInTheChainLeavesEarlierLevelsUsable) {
    const uint32_t frame_offset = 1;
    node_specs_.store(1, {pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    node_specs_.store(2, {pud_substitution{.var_idx = 1, .value = var(0)}});
    node_specs_.store(3, {pud_substitution{.var_idx = 1, .value = functor("b", {})}});
    node_specs_.store(4, {pud_substitution{.var_idx = 1, .value = functor("a", {})}});
    std::optional<replay_descent_t> opened = open({}, functor("p", {}), functor("p", {}), frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_two = opened->descend(1);
    ASSERT_TRUE(at_two.has_value());
    at_two = at_two->descend(2);
    ASSERT_TRUE(at_two.has_value());
    EXPECT_FALSE(at_two->descend(3).has_value());
    EXPECT_TRUE(at_two->descend(4).has_value());
}

TEST_F(PudReplayDescentIntegrationTest, AgreesWithTheBindingDescentAtFrameZero) {
    node_specs_.store(1, {
        pud_substitution{.var_idx = 2, .value = var(0)},
        pud_substitution{.var_idx = 1, .value = functor("f", {var(3)})},
    });
    node_specs_.store(2, {
        pud_substitution{.var_idx = 3, .value = functor("c", {})},
        pud_substitution{.var_idx = 4, .value = var(2)},
    });
    binding_descent_t binding = binding_descent_t{node_specs_, exprs, exprs}
        .descend(1)
        .descend(2);
    std::optional<replay_descent_t> replay = open({}, functor("p", {}), functor("p", {}), 0u);
    ASSERT_TRUE(replay.has_value());
    replay = replay->descend(1);
    ASSERT_TRUE(replay.has_value());
    replay = replay->descend(2);
    ASSERT_TRUE(replay.has_value());
    for (uint32_t index = 0; index < 6; ++index) {
        pud_transient_ubs_state transient{binding.ubs_state()};
        bind_map_t bind_map = transient.make_bind_map(exprs, exprs);
        EXPECT_EQ(resolve_global(*replay, index), bind_map.resolve(index)) << "var " << index;
    }
}

TEST_F(PudReplayDescentIntegrationTest, ParentUnchangedByChild) {
    const pud_node_id child = 1;
    node_specs_.store(child, {pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    std::optional<replay_descent_t> opened = open({}, var(0), var(0), 2u);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_child = opened->descend(child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(resolve_at_frame(*at_child, 2u, 0), functor("a", {}));
    EXPECT_EQ(resolve_at_frame(*opened, 2u, 0), var(0));
}

TEST_F(PudReplayDescentIntegrationTest, OpenedReplayIsAtTheRoot) {
    const expr* head = functor("main", {var(0)});
    std::optional<replay_descent_t> opened = open({}, head, head, 0);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->node(), root);
}

TEST_F(PudReplayDescentIntegrationTest, DescendedReplayIsAtTheChildAndTheParentStays) {
    const pud_node_id child = 1;
    const pud_node_id grandchild = 2;
    node_specs_.store(child, {});
    node_specs_.store(grandchild, {});
    const expr* head = functor("main", {var(0)});
    std::optional<replay_descent_t> opened = open({}, head, head, 0);
    ASSERT_TRUE(opened.has_value());
    std::optional<replay_descent_t> at_child = opened->descend(child);
    ASSERT_TRUE(at_child.has_value());
    std::optional<replay_descent_t> at_grandchild = at_child->descend(grandchild);
    ASSERT_TRUE(at_grandchild.has_value());
    EXPECT_EQ(at_child->node(), child);
    EXPECT_EQ(at_grandchild->node(), grandchild);
    EXPECT_EQ(opened->node(), root);
}
