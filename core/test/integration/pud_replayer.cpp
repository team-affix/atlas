#include <deque>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_modules.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "infrastructure/ubs_bind_map.hpp"

// The replayer over the real modules and the real term machinery. Every opened axiom is the only
// axiom of its own module; inferences are stored under the node each test descends from.
struct PudReplayerIntegrationTest : public ::testing::Test {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;
    using modules_t = pud_modules<pud_module>;

    static constexpr pud_module_id k_scratch_module = 0;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    term_factory_t terms{globalize, exprs, exprs};
    pud_module scratch{k_scratch_module, {}, {rule(exprs.make_functor(functors.id("dummy"), {}), {}, 0)}};
    std::deque<pud_module> opened_modules;
    modules_t modules;
    pud_replayer<modules_t, modules_t, expr_pool, term_factory_t, term_factory_t, term_factory_t> replayer{
        modules, modules, exprs, terms, terms, terms};
    pud_binder<term_factory_t, modules_t> binder{terms, modules};
    pud_global_node_id last_root{.module = k_scratch_module, .node = 0};

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    void SetUp() override {
        modules.register_module(scratch);
    }
};

TEST_F(PudReplayerIntegrationTest, SpecsReplayLiftedToFrameOffset) {
    const uint32_t frame_offset = 3;
    const expr* head = functor("main", {var(0), var(1), var(2)});
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {
            pud_substitution{.var_idx = 0, .value = functor("a", {})},
            pud_substitution{.var_idx = 1, .value = functor("b", {})},
        },
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> opened = replayer.enter({}, head, frame_offset, last_root);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    ubs_bind_map bind_map_child{at_child->ubs_state};
    EXPECT_EQ(terms.make_normalizer(bind_map_child).normalize(framed_expr{var(globalize.globalize(frame_offset, 0)), 0}), functor("a", {}));
    EXPECT_EQ(terms.make_normalizer(bind_map_child).normalize(framed_expr{var(globalize.globalize(frame_offset, 1)), 0}), functor("b", {}));
    EXPECT_EQ(terms.make_normalizer(bind_map_child).normalize(framed_expr{var(globalize.globalize(0u, 0)), 0}), functor("a", {}));
    EXPECT_EQ(terms.make_normalizer(bind_map_child).normalize(framed_expr{var(globalize.globalize(0u, 1)), 0}), functor("b", {}));
}

TEST_F(PudReplayerIntegrationTest, DifferentFrameOffsetsStayIndependent) {
    const expr* head = var(0);
    const pud_module_id module_one = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module_one, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    const pud_global_node_id root_one = modules.list_axioms(module_one).front();
    const pud_global_node_id child_one = modules.store_inference(module_one, pud_inference{
        .parent = root_one.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_module_id module_five = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module_five, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    const pud_global_node_id root_five = modules.list_axioms(module_five).front();
    const pud_global_node_id child_five = modules.store_inference(module_five, pud_inference{
        .parent = root_five.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> at_one = replayer.enter({}, var(0), 1u, root_one);
    std::optional<pud_replay> at_five = replayer.enter({}, var(0), 5u, root_five);
    ASSERT_TRUE(at_one.has_value());
    ASSERT_TRUE(at_five.has_value());
    std::optional<pud_replay> replay_child_one = replayer.descend(*at_one, child_one);
    std::optional<pud_replay> replay_child_five = replayer.descend(*at_five, child_five);
    ASSERT_TRUE(replay_child_one.has_value());
    ASSERT_TRUE(replay_child_five.has_value());
    ubs_bind_map bind_map_one{replay_child_one->ubs_state};
    ubs_bind_map bind_map_five{replay_child_five->ubs_state};
    EXPECT_EQ(terms.make_normalizer(bind_map_one).normalize(framed_expr{var(1u), 0}), functor("a", {}));
    EXPECT_EQ(terms.make_normalizer(bind_map_five).normalize(framed_expr{var(5u), 0}), functor("a", {}));
    const expr* resolved_one_at_five = terms.make_normalizer(bind_map_one).normalize(framed_expr{var(5u), 0});
    const expr::var* rep_var_one_at_five = std::get_if<expr::var>(&resolved_one_at_five->content);
    ASSERT_NE(rep_var_one_at_five, nullptr);
    EXPECT_EQ(rep_var_one_at_five->index, 5u);
    const expr* resolved_five_at_one = terms.make_normalizer(bind_map_five).normalize(framed_expr{var(1u), 0});
    const expr::var* rep_var_five_at_one = std::get_if<expr::var>(&resolved_five_at_one->content);
    ASSERT_NE(rep_var_five_at_one, nullptr);
    EXPECT_EQ(rep_var_five_at_one->index, 1u);
}

TEST_F(PudReplayerIntegrationTest, OpeningFlowFromBindingDescentState) {
    const uint32_t frame_offset = 1u;
    const pud_global_node_id binding_child = modules.store_inference(k_scratch_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_ubs_state caller_binding = binder.bind(pud_ubs_state{}, binding_child);

    const expr* head = var(0);
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id agree_child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id contradict_child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("b", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> opened = replayer.enter(caller_binding, var(0), frame_offset, last_root);
    ASSERT_TRUE(opened.has_value());

    EXPECT_TRUE(replayer.descend(*opened, agree_child).has_value());

    EXPECT_FALSE(replayer.descend(*opened, contradict_child).has_value());
}

TEST_F(PudReplayerIntegrationTest, ContradictsInheritedCallerBindingRefutes) {
    const expr* head = var(0);
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("b", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> opened = replayer.enter({}, functor("a", {}), 1, last_root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_FALSE(replayer.descend(*opened, child).has_value());
}

TEST_F(PudReplayerIntegrationTest, ContradictsEarlierReplayedSpecRefutes) {
    const expr* head = functor("main", {});
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id good_child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id bad_child = modules.store_inference(module, pud_inference{
        .parent = good_child.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("x", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> opened = replayer.enter({}, head, 0u, last_root);
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
    const expr* head = functor("main", {});
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = std::move(substitutions),
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> opened = replayer.enter({}, head, 0u, last_root);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    ubs_bind_map bind_map{at_child->ubs_state};
    for (uint32_t substitution_index = 0; substitution_index < substitution_count; ++substitution_index) {
        std::string label = "v" + std::to_string(substitution_index);
        EXPECT_EQ(terms.make_normalizer(bind_map).normalize(framed_expr{var(globalize.globalize(0u, substitution_index)), 0}), functor(label.c_str(), {}));
    }
}

TEST_F(PudReplayerIntegrationTest, ForkedDescentsFromSharedPrefixStayIndependent) {
    const uint32_t frame_offset = 3;
    const expr* head = functor("parent", {var(0), var(1)});
    const expr* query = functor("parent", {var(0), var(2)});
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id bob_child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {
            pud_substitution{.var_idx = 0, .value = functor("tom", {})},
            pud_substitution{.var_idx = 1, .value = functor("bob", {})},
        },
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id x_child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {
            pud_substitution{.var_idx = 0, .value = functor("tom", {})},
            pud_substitution{.var_idx = 1, .value = functor("x", {})},
        },
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> opened = replayer.enter({}, query, frame_offset, last_root);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_bob = replayer.descend(*opened, bob_child);
    std::optional<pud_replay> at_x = replayer.descend(*opened, x_child);
    ASSERT_TRUE(at_bob.has_value());
    ASSERT_TRUE(at_x.has_value());
    ubs_bind_map bind_map_bob{at_bob->ubs_state};
    ubs_bind_map bind_map_x{at_x->ubs_state};
    EXPECT_EQ(terms.make_normalizer(bind_map_bob).normalize(framed_expr{var(globalize.globalize(frame_offset, 1)), 0}), functor("bob", {}));
    EXPECT_EQ(terms.make_normalizer(bind_map_x).normalize(framed_expr{var(globalize.globalize(frame_offset, 1)), 0}), functor("x", {}));
}

TEST_F(PudReplayerIntegrationTest, CallerVarThroughHeadBindingSeesCalleeFunctor) {
    const uint32_t frame_offset = 3;
    const expr* head = functor("parent", {var(0), var(1)});
    const expr* query = functor("parent", {var(0), var(2)});
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {
            pud_substitution{.var_idx = 0, .value = functor("tom", {})},
            pud_substitution{.var_idx = 1, .value = functor("bob", {})},
        },
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> opened = replayer.enter({}, query, frame_offset, last_root);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    ubs_bind_map bind_map{at_child->ubs_state};
    EXPECT_EQ(terms.make_normalizer(bind_map).normalize(framed_expr{var(globalize.globalize(0u, 2)), 0}), functor("bob", {}));
}

TEST_F(PudReplayerIntegrationTest, VarSpecEquatesCalleeVarsAtTheFrameOffset) {
    const uint32_t frame_offset = 3;
    const expr* head = functor("p", {});
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = var(0)}},
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> opened = replayer.enter({}, head, frame_offset, last_root);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    ubs_bind_map bind_map{at_child->ubs_state};
    EXPECT_EQ(
        terms.make_normalizer(bind_map).normalize(framed_expr{var(globalize.globalize(frame_offset, 1)), 0}),
        terms.make_normalizer(bind_map).normalize(framed_expr{var(globalize.globalize(frame_offset, 0)), 0}));
    const uint32_t global_at_frame = globalize.globalize(frame_offset, 0);
    const expr* resolved_global_at_frame = terms.make_normalizer(bind_map).normalize(framed_expr{var(global_at_frame), 0});
    const expr::var* rep_var_global_at_frame = std::get_if<expr::var>(&resolved_global_at_frame->content);
    ASSERT_NE(rep_var_global_at_frame, nullptr);
    EXPECT_EQ(rep_var_global_at_frame->index, global_at_frame);
    const expr* resolved_zero = terms.make_normalizer(bind_map).normalize(framed_expr{var(0u), 0});
    const expr::var* rep_var_zero = std::get_if<expr::var>(&resolved_zero->content);
    ASSERT_NE(rep_var_zero, nullptr);
    EXPECT_EQ(rep_var_zero->index, 0u);
    const expr* resolved_one = terms.make_normalizer(bind_map).normalize(framed_expr{var(1u), 0});
    const expr::var* rep_var_one = std::get_if<expr::var>(&resolved_one->content);
    ASSERT_NE(rep_var_one, nullptr);
    EXPECT_EQ(rep_var_one->index, 1u);
}

TEST_F(PudReplayerIntegrationTest, SpecValueVarsAreLiftedToTheFrameOffset) {
    const uint32_t frame_offset = 2;
    const expr* head = functor("p", {});
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("f", {var(1)})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id grandchild = modules.store_inference(module, pud_inference{
        .parent = child.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = functor("b", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> opened = replayer.enter({}, head, frame_offset, last_root);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    ubs_bind_map bind_map_child{at_child->ubs_state};
    const expr* resolved_child_one = terms.make_normalizer(bind_map_child).normalize(framed_expr{var(1u), 0});
    const expr::var* rep_var_child_one = std::get_if<expr::var>(&resolved_child_one->content);
    ASSERT_NE(rep_var_child_one, nullptr);
    EXPECT_EQ(rep_var_child_one->index, 1u);
    std::optional<pud_replay> at_grandchild = replayer.descend(*at_child, grandchild);
    ASSERT_TRUE(at_grandchild.has_value());
    ubs_bind_map bind_map_grandchild{at_grandchild->ubs_state};
    EXPECT_EQ(terms.make_normalizer(bind_map_grandchild).normalize(framed_expr{var(globalize.globalize(frame_offset, 1)), 0}), functor("b", {}));
    const expr* resolved_grandchild_one = terms.make_normalizer(bind_map_grandchild).normalize(framed_expr{var(1u), 0});
    const expr::var* rep_var_grandchild_one = std::get_if<expr::var>(&resolved_grandchild_one->content);
    ASSERT_NE(rep_var_grandchild_one, nullptr);
    EXPECT_EQ(rep_var_grandchild_one->index, 1u);
}

TEST_F(PudReplayerIntegrationTest, SiblingStillDescendsAfterARefutedChild) {
    const expr* head = var(0);
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id bad_child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("b", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id good_child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> opened = replayer.enter({}, functor("a", {}), 1, last_root);
    ASSERT_TRUE(opened.has_value());
    ASSERT_FALSE(replayer.descend(*opened, bad_child).has_value());
    std::optional<pud_replay> at_good = replayer.descend(*opened, good_child);
    ASSERT_TRUE(at_good.has_value());
    ubs_bind_map bind_map{at_good->ubs_state};
    EXPECT_EQ(terms.make_normalizer(bind_map).normalize(framed_expr{var(globalize.globalize(1, 0)), 0}), functor("a", {}));
}

TEST_F(PudReplayerIntegrationTest, NodeWithNoSpecsKeepsTheBindings) {
    const expr* head = var(0);
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> opened = replayer.enter({}, functor("a", {}), 4, last_root);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    ubs_bind_map bind_map{at_child->ubs_state};
    EXPECT_EQ(terms.make_normalizer(bind_map).normalize(framed_expr{var(globalize.globalize(4, 0)), 0}), functor("a", {}));
}

TEST_F(PudReplayerIntegrationTest, ThreeLevelReplayAccumulates) {
    const uint32_t frame_offset = 5;
    const expr* head = functor("p", {});
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id first = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("f", {var(1)})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id second = modules.store_inference(module, pud_inference{
        .parent = first.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = functor("g", {var(2)})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id third = modules.store_inference(module, pud_inference{
        .parent = second.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 2, .value = functor("c", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> at = replayer.enter({}, head, frame_offset, last_root);
    for (pud_global_node_id level_node : {first, second, third}) {
        ASSERT_TRUE(at.has_value()) << "node " << level_node.node;
        at = replayer.descend(*at, level_node);
    }
    ASSERT_TRUE(at.has_value());
    ubs_bind_map bind_map{at->ubs_state};
    EXPECT_EQ(terms.make_normalizer(bind_map).normalize(framed_expr{var(globalize.globalize(frame_offset, 2)), 0}), functor("c", {}));
    const expr* resolved_one = terms.make_normalizer(bind_map).normalize(framed_expr{var(globalize.globalize(frame_offset, 1)), 0});
    const expr::functor* g_functor = std::get_if<expr::functor>(&resolved_one->content);
    ASSERT_NE(g_functor, nullptr);
    EXPECT_EQ(g_functor->id, functors.id("g"));
}

TEST_F(PudReplayerIntegrationTest, RefutationDeepInTheChainLeavesEarlierLevelsUsable) {
    const uint32_t frame_offset = 1;
    const expr* head = functor("p", {});
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id first = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id second = modules.store_inference(module, pud_inference{
        .parent = first.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = var(0)}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id contradicting = modules.store_inference(module, pud_inference{
        .parent = second.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = functor("b", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id agreeing = modules.store_inference(module, pud_inference{
        .parent = second.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> opened = replayer.enter({}, head, frame_offset, last_root);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_two = replayer.descend(*opened, first);
    ASSERT_TRUE(at_two.has_value());
    at_two = replayer.descend(*at_two, second);
    ASSERT_TRUE(at_two.has_value());
    EXPECT_FALSE(replayer.descend(*at_two, contradicting).has_value());
    EXPECT_TRUE(replayer.descend(*at_two, agreeing).has_value());
}

TEST_F(PudReplayerIntegrationTest, AgreesWithTheBindingDescentAtFrameZero) {
    const expr* head = functor("p", {});
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id first = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {
            pud_substitution{.var_idx = 2, .value = var(0)},
            pud_substitution{.var_idx = 1, .value = functor("f", {var(3)})},
        },
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id second = modules.store_inference(module, pud_inference{
        .parent = first.node,
        .call_site = 0,
        .substitutions = {
            pud_substitution{.var_idx = 3, .value = functor("c", {})},
            pud_substitution{.var_idx = 4, .value = var(2)},
        },
        .body_goals = {},
        .var_count = 0,
    });
    const pud_ubs_state binding = binder.bind(binder.bind(pud_ubs_state{}, first), second);
    std::optional<pud_replay> replay = replayer.enter({}, head, 0u, last_root);
    ASSERT_TRUE(replay.has_value());
    replay = replayer.descend(*replay, first);
    ASSERT_TRUE(replay.has_value());
    replay = replayer.descend(*replay, second);
    ASSERT_TRUE(replay.has_value());
    for (uint32_t index = 0; index < 6; ++index) {
        ubs_bind_map bind_map{binding};
        ubs_bind_map replay_bind_map{replay->ubs_state};
        EXPECT_EQ(
            terms.make_normalizer(replay_bind_map).normalize(framed_expr{var(index), 0}),
            terms.make_normalizer(bind_map).normalize(framed_expr{var(index), 0}))
            << "var " << index;
    }
}

TEST_F(PudReplayerIntegrationTest, ParentUnchangedByChild) {
    const expr* head = var(0);
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> opened = replayer.enter({}, head, 2u, last_root);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    ubs_bind_map bind_map_child{at_child->ubs_state};
    ubs_bind_map bind_map_opened{opened->ubs_state};
    EXPECT_EQ(terms.make_normalizer(bind_map_child).normalize(framed_expr{var(globalize.globalize(2u, 0)), 0}), functor("a", {}));
    EXPECT_EQ(terms.make_normalizer(bind_map_opened).normalize(framed_expr{var(globalize.globalize(2u, 0)), 0}), var(0));
}

TEST_F(PudReplayerIntegrationTest, OpenedReplayIsAtTheRoot) {
    const expr* head = functor("main", {var(0)});
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    std::optional<pud_replay> opened = replayer.enter({}, head, 0u, last_root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->node, last_root);
}

TEST_F(PudReplayerIntegrationTest, DescendedReplayIsAtTheChildAndTheParentStays) {
    const expr* head = functor("main", {var(0)});
    const pud_module_id module = k_scratch_module + 1 + opened_modules.size();
    opened_modules.emplace_back(module, std::vector<pud_module_id>{}, std::vector<rule>{rule(head, {}, 0)});
    modules.register_module(opened_modules.back());
    last_root = modules.list_axioms(module).front();
    const pud_global_node_id child = modules.store_inference(module, pud_inference{
        .parent = last_root.node,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id grandchild = modules.store_inference(module, pud_inference{
        .parent = child.node,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {},
        .var_count = 0,
    });
    std::optional<pud_replay> opened = replayer.enter({}, head, 0u, last_root);
    ASSERT_TRUE(opened.has_value());
    std::optional<pud_replay> at_child = replayer.descend(*opened, child);
    ASSERT_TRUE(at_child.has_value());
    std::optional<pud_replay> at_grandchild = replayer.descend(*at_child, grandchild);
    ASSERT_TRUE(at_grandchild.has_value());
    EXPECT_EQ(at_child->node, child);
    EXPECT_EQ(at_grandchild->node, grandchild);
    EXPECT_EQ(opened->node, last_root);
}
