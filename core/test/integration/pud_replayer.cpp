#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_data.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "infrastructure/ubs_bind_map.hpp"

// The replayer over the real database and the real term machinery.
struct PudReplayerIntegrationTest : public ::testing::Test {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    term_factory_t terms{globalize, exprs, exprs};
    pud_data data;
    pud_replayer<pud_data, pud_data, expr_pool, term_factory_t, term_factory_t, term_factory_t> replayer{
        data, data, exprs, terms, terms, terms};
    pud_binder<term_factory_t, pud_data> binder{terms, data};
    pud_node_id dummy_root = 0;
    pud_node_id last_root = 0;

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    void SetUp() override {
        dummy_root = data.store_axiom(rule(functor("dummy", {}), {}, 0));
    }

    pud_node_id infer(std::vector<pud_substitution> substitutions) {
        return data.store_inference(pud_node_delta{
            .parent = dummy_root,
            .call_site = 0,
            .substitutions = std::move(substitutions),
            .body_goals = {},
            .var_count = 0,
        });
    }

    std::optional<pud_replay> open(
        const pud_ubs_state& caller_state,
        const expr* query_expr,
        const expr* axiom_head,
        uint32_t frame_offset) {
        last_root = data.store_axiom(rule(axiom_head, {}, 0));
        return replayer.enter(caller_state, query_expr, frame_offset, last_root);
    }

    const expr* resolve_global(const pud_replay& replay, uint32_t global_var_index) {
        ubs_bind_map bind_map{replay.ubs_state};
        return terms.make_normalizer(bind_map).normalize(framed_expr{var(global_var_index), 0});
    }

    const expr* resolve_at_frame(const pud_replay& replay, uint32_t frame_offset, uint32_t local_var_index) {
        return resolve_global(replay, globalize.globalize(frame_offset, local_var_index));
    }

    void expect_unbound_global(const pud_replay& replay, uint32_t global_var_index) {
        const expr* resolved = resolve_global(replay, global_var_index);
        const expr::var* rep_var = std::get_if<expr::var>(&resolved->content);
        ASSERT_NE(rep_var, nullptr);
        EXPECT_EQ(rep_var->index, global_var_index);
    }
};

TEST_F(PudReplayerIntegrationTest, SpecsReplayLiftedToFrameOffset) {
    const pud_node_id child = infer({
        pud_substitution{.var_idx = 0, .value = functor("a", {})},
        pud_substitution{.var_idx = 1, .value = functor("b", {})},
    });
    const uint32_t frame_offset = 3;
    const expr* head = functor("main", {var(0), var(1), var(2)});
    std::optional<pud_replay> opened = open({}, head, head, frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(resolve_at_frame(*at_child, frame_offset, 0), functor("a", {}));
    EXPECT_EQ(resolve_at_frame(*at_child, frame_offset, 1), functor("b", {}));
    EXPECT_EQ(resolve_at_frame(*at_child, 0u, 0), functor("a", {}));
    EXPECT_EQ(resolve_at_frame(*at_child, 0u, 1), functor("b", {}));
}

TEST_F(PudReplayerIntegrationTest, DifferentFrameOffsetsStayIndependent) {
    const pud_node_id child = infer({pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    const expr* head = var(0);
    std::optional<pud_replay> at_one = open({}, var(0), head, 1u);
    std::optional<pud_replay> at_five = open({}, var(0), head, 5u);
    ASSERT_TRUE(at_one.has_value());
    ASSERT_TRUE(at_five.has_value());
    std::optional<pud_replay> child_one = replayer.descend(*at_one, child);
    std::optional<pud_replay> child_five = replayer.descend(*at_five, child);
    ASSERT_TRUE(child_one.has_value());
    ASSERT_TRUE(child_five.has_value());
    EXPECT_EQ(resolve_global(*child_one, 1u), functor("a", {}));
    EXPECT_EQ(resolve_global(*child_five, 5u), functor("a", {}));
    expect_unbound_global(*child_one, 5u);
    expect_unbound_global(*child_five, 1u);
}

TEST_F(PudReplayerIntegrationTest, OpeningFlowFromBindingDescentState) {
    const uint32_t frame_offset = 1u;
    const pud_node_id binding_child = infer({pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    const pud_ubs_state caller_binding = binder.bind(pud_ubs_state{}, binding_child);

    std::optional<pud_replay> opened = open(caller_binding, var(0), var(0), frame_offset);
    ASSERT_TRUE(opened.has_value());

    const pud_node_id agree_child = infer({pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    EXPECT_TRUE(replayer.descend(*opened, agree_child).has_value());

    const pud_node_id contradict_child = infer({pud_substitution{.var_idx = 0, .value = functor("b", {})}});
    EXPECT_FALSE(replayer.descend(*opened, contradict_child).has_value());
}

TEST_F(PudReplayerIntegrationTest, ContradictsInheritedCallerBindingRefutes) {
    const pud_node_id child = infer({pud_substitution{.var_idx = 0, .value = functor("b", {})}});
    std::optional<pud_replay> opened = open({}, functor("a", {}), var(0), 1);
    ASSERT_TRUE(opened.has_value());
    EXPECT_FALSE(replayer.descend(*opened, child).has_value());
}

TEST_F(PudReplayerIntegrationTest, ContradictsEarlierReplayedSpecRefutes) {
    const pud_node_id good_child = infer({pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    const pud_node_id bad_child = infer({pud_substitution{.var_idx = 0, .value = functor("x", {})}});
    std::optional<pud_replay> opened = open({}, functor("main", {}), functor("main", {}), 0u);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_good = replayer.descend(*opened, good_child);
    ASSERT_TRUE(at_good.has_value());
    EXPECT_FALSE(replayer.descend(*at_good, bad_child).has_value());
}

TEST_F(PudReplayerIntegrationTest, ManySpecsReplay) {
    const uint32_t substitution_count = 200;
    std::vector<pud_substitution> substitutions;
    for (uint32_t substitution_index = 0; substitution_index < substitution_count; ++substitution_index) {
        std::string label = "v" + std::to_string(substitution_index);
        substitutions.push_back(pud_substitution{.var_idx = substitution_index, .value = functor(label.c_str(), {})});
    }
    const pud_node_id child = infer(std::move(substitutions));
    std::optional<pud_replay> opened = open({}, functor("main", {}), functor("main", {}), 0u);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    for (uint32_t substitution_index = 0; substitution_index < substitution_count; ++substitution_index) {
        std::string label = "v" + std::to_string(substitution_index);
        EXPECT_EQ(resolve_at_frame(*at_child, 0u, substitution_index), functor(label.c_str(), {}));
    }
}

TEST_F(PudReplayerIntegrationTest, ForkedDescentsFromSharedPrefixStayIndependent) {
    const uint32_t frame_offset = 3;
    const pud_node_id bob_child = infer({
        pud_substitution{.var_idx = 0, .value = functor("tom", {})},
        pud_substitution{.var_idx = 1, .value = functor("bob", {})},
    });
    const pud_node_id x_child = infer({
        pud_substitution{.var_idx = 0, .value = functor("tom", {})},
        pud_substitution{.var_idx = 1, .value = functor("x", {})},
    });
    const expr* head = functor("parent", {var(0), var(1)});
    const expr* query = functor("parent", {var(0), var(2)});
    std::optional<pud_replay> opened = open({}, query, head, frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_bob = replayer.descend(*opened, bob_child);
    std::optional<pud_replay> at_x = replayer.descend(*opened, x_child);
    ASSERT_TRUE(at_bob.has_value());
    ASSERT_TRUE(at_x.has_value());
    EXPECT_EQ(resolve_at_frame(*at_bob, frame_offset, 1), functor("bob", {}));
    EXPECT_EQ(resolve_at_frame(*at_x, frame_offset, 1), functor("x", {}));
}

TEST_F(PudReplayerIntegrationTest, CallerVarThroughHeadBindingSeesCalleeFunctor) {
    const uint32_t frame_offset = 3;
    const pud_node_id child = infer({
        pud_substitution{.var_idx = 0, .value = functor("tom", {})},
        pud_substitution{.var_idx = 1, .value = functor("bob", {})},
    });
    const expr* head = functor("parent", {var(0), var(1)});
    const expr* query = functor("parent", {var(0), var(2)});
    std::optional<pud_replay> opened = open({}, query, head, frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(resolve_at_frame(*at_child, 0u, 2), functor("bob", {}));
}

TEST_F(PudReplayerIntegrationTest, VarSpecEquatesCalleeVarsAtTheFrameOffset) {
    const uint32_t frame_offset = 3;
    const pud_node_id child = infer({pud_substitution{.var_idx = 1, .value = var(0)}});
    std::optional<pud_replay> opened = open({}, functor("p", {}), functor("p", {}), frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(resolve_at_frame(*at_child, frame_offset, 1), resolve_at_frame(*at_child, frame_offset, 0));
    expect_unbound_global(*at_child, globalize.globalize(frame_offset, 0));
    expect_unbound_global(*at_child, 0u);
    expect_unbound_global(*at_child, 1u);
}

TEST_F(PudReplayerIntegrationTest, SpecValueVarsAreLiftedToTheFrameOffset) {
    const uint32_t frame_offset = 2;
    const pud_node_id child = infer({pud_substitution{.var_idx = 0, .value = functor("f", {var(1)})}});
    const pud_node_id grandchild = infer({pud_substitution{.var_idx = 1, .value = functor("b", {})}});
    std::optional<pud_replay> opened = open({}, functor("p", {}), functor("p", {}), frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    expect_unbound_global(*at_child, 1u);
    std::optional<pud_replay> at_grandchild = replayer.descend(*at_child, grandchild);
    ASSERT_TRUE(at_grandchild.has_value());
    EXPECT_EQ(resolve_at_frame(*at_grandchild, frame_offset, 1), functor("b", {}));
    expect_unbound_global(*at_grandchild, 1u);
}

TEST_F(PudReplayerIntegrationTest, SiblingStillDescendsAfterARefutedChild) {
    const pud_node_id bad_child = infer({pud_substitution{.var_idx = 0, .value = functor("b", {})}});
    const pud_node_id good_child = infer({pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    std::optional<pud_replay> opened = open({}, functor("a", {}), var(0), 1);
    ASSERT_TRUE(opened.has_value());
    ASSERT_FALSE(replayer.descend(*opened, bad_child).has_value());
    std::optional<pud_replay> at_good = replayer.descend(*opened, good_child);
    ASSERT_TRUE(at_good.has_value());
    EXPECT_EQ(resolve_at_frame(*at_good, 1, 0), functor("a", {}));
}

TEST_F(PudReplayerIntegrationTest, NodeWithNoSpecsKeepsTheBindings) {
    const pud_node_id child = infer({});
    std::optional<pud_replay> opened = open({}, functor("a", {}), var(0), 4);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(resolve_at_frame(*at_child, 4, 0), functor("a", {}));
}

TEST_F(PudReplayerIntegrationTest, ThreeLevelReplayAccumulates) {
    const uint32_t frame_offset = 5;
    const pud_node_id first = infer({pud_substitution{.var_idx = 0, .value = functor("f", {var(1)})}});
    const pud_node_id second = infer({pud_substitution{.var_idx = 1, .value = functor("g", {var(2)})}});
    const pud_node_id third = infer({pud_substitution{.var_idx = 2, .value = functor("c", {})}});
    std::optional<pud_replay> at = open({}, functor("p", {}), functor("p", {}), frame_offset);
    for (pud_node_id level_node : {first, second, third}) {
        ASSERT_TRUE(at.has_value()) << "node " << level_node;
        at = replayer.descend(*at, level_node);
    }
    ASSERT_TRUE(at.has_value());
    EXPECT_EQ(resolve_at_frame(*at, frame_offset, 2), functor("c", {}));
    const expr* resolved_one = resolve_at_frame(*at, frame_offset, 1);
    const expr::functor* g_functor = std::get_if<expr::functor>(&resolved_one->content);
    ASSERT_NE(g_functor, nullptr);
    EXPECT_EQ(g_functor->id, functors.id("g"));
}

TEST_F(PudReplayerIntegrationTest, RefutationDeepInTheChainLeavesEarlierLevelsUsable) {
    const uint32_t frame_offset = 1;
    const pud_node_id first = infer({pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    const pud_node_id second = infer({pud_substitution{.var_idx = 1, .value = var(0)}});
    const pud_node_id contradicting = infer({pud_substitution{.var_idx = 1, .value = functor("b", {})}});
    const pud_node_id agreeing = infer({pud_substitution{.var_idx = 1, .value = functor("a", {})}});
    std::optional<pud_replay> opened = open({}, functor("p", {}), functor("p", {}), frame_offset);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_two = replayer.descend(*opened, first);
    ASSERT_TRUE(at_two.has_value());
    at_two = replayer.descend(*at_two, second);
    ASSERT_TRUE(at_two.has_value());
    EXPECT_FALSE(replayer.descend(*at_two, contradicting).has_value());
    EXPECT_TRUE(replayer.descend(*at_two, agreeing).has_value());
}

TEST_F(PudReplayerIntegrationTest, AgreesWithTheBindingDescentAtFrameZero) {
    const pud_node_id first = infer({
        pud_substitution{.var_idx = 2, .value = var(0)},
        pud_substitution{.var_idx = 1, .value = functor("f", {var(3)})},
    });
    const pud_node_id second = infer({
        pud_substitution{.var_idx = 3, .value = functor("c", {})},
        pud_substitution{.var_idx = 4, .value = var(2)},
    });
    const pud_ubs_state binding = binder.bind(binder.bind(pud_ubs_state{}, first), second);
    std::optional<pud_replay> replay = open({}, functor("p", {}), functor("p", {}), 0u);
    ASSERT_TRUE(replay.has_value());
    replay = replayer.descend(*replay, first);
    ASSERT_TRUE(replay.has_value());
    replay = replayer.descend(*replay, second);
    ASSERT_TRUE(replay.has_value());
    for (uint32_t index = 0; index < 6; ++index) {
        ubs_bind_map bind_map{binding};
        EXPECT_EQ(resolve_global(*replay, index), terms.make_normalizer(bind_map).normalize(framed_expr{var(index), 0}))
            << "var " << index;
    }
}

TEST_F(PudReplayerIntegrationTest, ParentUnchangedByChild) {
    const pud_node_id child = infer({pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    std::optional<pud_replay> opened = open({}, var(0), var(0), 2u);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    EXPECT_EQ(resolve_at_frame(*at_child, 2u, 0), functor("a", {}));
    EXPECT_EQ(resolve_at_frame(*opened, 2u, 0), var(0));
}

TEST_F(PudReplayerIntegrationTest, OpenedReplayIsAtTheRoot) {
    const expr* head = functor("main", {var(0)});
    std::optional<pud_replay> opened = open({}, head, head, 0);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->node, last_root);
}

TEST_F(PudReplayerIntegrationTest, DescendedReplayIsAtTheChildAndTheParentStays) {
    const pud_node_id child = infer({});
    const pud_node_id grandchild = infer({});
    const expr* head = functor("main", {var(0)});
    std::optional<pud_replay> opened = open({}, head, head, 0);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    std::optional<pud_replay> at_grandchild = replayer.descend(*at_child, grandchild);
    ASSERT_TRUE(at_grandchild.has_value());
    EXPECT_EQ(at_child->node, child);
    EXPECT_EQ(at_grandchild->node, grandchild);
    EXPECT_EQ(opened->node, last_root);
}
