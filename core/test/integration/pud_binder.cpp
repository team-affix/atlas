#include <optional>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_modules.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "infrastructure/ubs_bind_map.hpp"

inline const expr* expr_from_framed_functor(expr_pool& pool, const framed_functor& ff) {
    const expr::functor& f = *ff.functor;
    return pool.make_functor(f.id, f.args);
}

// The binder over the real modules and real bind maps. Every inference hangs under the one dummy
// axiom of the test's module.
struct PudBinderIntegrationTest : public ::testing::Test {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;
    using modules_t = pud_modules<pud_module>;

    static constexpr pud_module_id k_module = 0;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    term_factory_t terms{globalize, exprs, exprs};
    pud_module data{k_module, {}, {rule(exprs.make_functor(functors.id("dummy"), {}), {}, 0)}};
    modules_t modules;
    pud_binder<term_factory_t, modules_t> binder{terms, modules};

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    void SetUp() override {
        modules.register_module(data);
    }
};

TEST_F(PudBinderIntegrationTest, RootHasNoBindings) {
    const ubs_class class_zero = ubs_bind_map{pud_ubs_state{}}.find(0);
    const expr* resolved_zero = class_zero.value.has_value() ? expr_from_framed_functor(exprs, class_zero.value.value()) : exprs.make_var(class_zero.rep);
    EXPECT_EQ(resolved_zero, var(0));
    const ubs_class class_three = ubs_bind_map{pud_ubs_state{}}.find(3);
    const expr* resolved_three = class_three.value.has_value() ? expr_from_framed_functor(exprs, class_three.value.value()) : exprs.make_var(class_three.rep);
    EXPECT_EQ(resolved_three, var(3));
}

TEST_F(PudBinderIntegrationTest, FunctorSpecInstantiates) {
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_ubs_state bound = binder.bind(pud_ubs_state{}, child);
    const ubs_class var_class = ubs_bind_map{bound}.find(0);
    if (var_class.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class.value.value()), functor("a", {}));
    else
        EXPECT_EQ(exprs.make_var(var_class.rep), functor("a", {}));
}

TEST_F(PudBinderIntegrationTest, VarSpecEquates) {
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = var(0)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_ubs_state at_child = binder.bind(pud_ubs_state{}, child);
    const ubs_class var_class_zero = ubs_bind_map{at_child}.find(0);
    const ubs_class var_class_one = ubs_bind_map{at_child}.find(1);
    if (var_class_zero.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_zero.value.value()), var(0));
    else
        EXPECT_EQ(exprs.make_var(var_class_zero.rep), var(0));
    if (var_class_one.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_one.value.value()), var(0));
    else
        EXPECT_EQ(exprs.make_var(var_class_one.rep), var(0));
}

TEST_F(PudBinderIntegrationTest, MultiLevelChainResolves) {
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = var(0)}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_ubs_state at_grandchild = binder.bind(binder.bind(pud_ubs_state{}, child), grandchild);
    const ubs_class var_class_zero = ubs_bind_map{at_grandchild}.find(0);
    const ubs_class var_class_one = ubs_bind_map{at_grandchild}.find(1);
    if (var_class_zero.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_zero.value.value()), functor("a", {}));
    else
        EXPECT_EQ(exprs.make_var(var_class_zero.rep), functor("a", {}));
    if (var_class_one.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_one.value.value()), functor("a", {}));
    else
        EXPECT_EQ(exprs.make_var(var_class_one.rep), functor("a", {}));
}

TEST_F(PudBinderIntegrationTest, ParentUnchangedByChild) {
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_ubs_state root_bindings = pud_ubs_state{};
    binder.bind(root_bindings, child);
    const ubs_class var_class = ubs_bind_map{root_bindings}.find(0);
    if (var_class.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class.value.value()), var(0));
    else
        EXPECT_EQ(exprs.make_var(var_class.rep), var(0));
}

TEST_F(PudBinderIntegrationTest, SiblingsDescendIndependently) {
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id sibling = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("b", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_ubs_state root_bindings = pud_ubs_state{};
    const pud_ubs_state at_child = binder.bind(root_bindings, child);
    const pud_ubs_state at_sibling = binder.bind(root_bindings, sibling);
    const ubs_class child_class = ubs_bind_map{at_child}.find(0);
    const ubs_class sibling_class = ubs_bind_map{at_sibling}.find(0);
    if (child_class.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, child_class.value.value()), functor("a", {}));
    else
        EXPECT_EQ(exprs.make_var(child_class.rep), functor("a", {}));
    if (sibling_class.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, sibling_class.value.value()), functor("b", {}));
    else
        EXPECT_EQ(exprs.make_var(sibling_class.rep), functor("b", {}));
}

TEST_F(PudBinderIntegrationTest, SeveralSpecsAtOneNodeApplyInOrder) {
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {
            pud_substitution{.var_idx = 1, .value = var(0)},
            pud_substitution{.var_idx = 0, .value = functor("a", {})},
        },
        .body_goals = {},
        .var_count = 0,
    });
    pud_ubs_state at_child = binder.bind(pud_ubs_state{}, child);
    const ubs_class var_class_zero = ubs_bind_map{at_child}.find(0);
    const ubs_class var_class_one = ubs_bind_map{at_child}.find(1);
    if (var_class_zero.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_zero.value.value()), functor("a", {}));
    else
        EXPECT_EQ(exprs.make_var(var_class_zero.rep), functor("a", {}));
    if (var_class_one.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_one.value.value()), functor("a", {}));
    else
        EXPECT_EQ(exprs.make_var(var_class_one.rep), functor("a", {}));
}

TEST_F(PudBinderIntegrationTest, InstantiationWithVarArgsLeavesTheArgsUnbound) {
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("f", {var(1), var(2)})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_ubs_state at_child = binder.bind(pud_ubs_state{}, child);
    const ubs_class var_class_zero = ubs_bind_map{at_child}.find(0);
    const ubs_class var_class_one = ubs_bind_map{at_child}.find(1);
    const ubs_class var_class_two = ubs_bind_map{at_child}.find(2);
    if (var_class_zero.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_zero.value.value()), functor("f", {var(1), var(2)}));
    else
        EXPECT_EQ(exprs.make_var(var_class_zero.rep), functor("f", {var(1), var(2)}));
    if (var_class_one.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_one.value.value()), var(1));
    else
        EXPECT_EQ(exprs.make_var(var_class_one.rep), var(1));
    if (var_class_two.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_two.value.value()), var(2));
    else
        EXPECT_EQ(exprs.make_var(var_class_two.rep), var(2));
}

TEST_F(PudBinderIntegrationTest, BindingAnArgLaterIsSeenThroughTheFunctor) {
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("f", {var(1)})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = functor("b", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_ubs_state at_grandchild = binder.bind(binder.bind(pud_ubs_state{}, child), grandchild);
    const ubs_class var_class_zero = ubs_bind_map{at_grandchild}.find(0);
    const ubs_class var_class_one = ubs_bind_map{at_grandchild}.find(1);
    if (var_class_zero.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_zero.value.value()), functor("f", {var(1)}));
    else
        EXPECT_EQ(exprs.make_var(var_class_zero.rep), functor("f", {var(1)}));
    if (var_class_one.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_one.value.value()), functor("b", {}));
    else
        EXPECT_EQ(exprs.make_var(var_class_one.rep), functor("b", {}));
}

TEST_F(PudBinderIntegrationTest, EquatedClassKeepsTheSmallestIndexAsRepresentative) {
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 5, .value = var(3)}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 3, .value = var(1)}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_ubs_state at_grandchild = binder.bind(binder.bind(pud_ubs_state{}, child), grandchild);
    const ubs_class var_class_five = ubs_bind_map{at_grandchild}.find(5);
    const ubs_class var_class_three = ubs_bind_map{at_grandchild}.find(3);
    const ubs_class var_class_one = ubs_bind_map{at_grandchild}.find(1);
    if (var_class_five.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_five.value.value()), var(1));
    else
        EXPECT_EQ(exprs.make_var(var_class_five.rep), var(1));
    if (var_class_three.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_three.value.value()), var(1));
    else
        EXPECT_EQ(exprs.make_var(var_class_three.rep), var(1));
    if (var_class_one.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_one.value.value()), var(1));
    else
        EXPECT_EQ(exprs.make_var(var_class_one.rep), var(1));
}

TEST_F(PudBinderIntegrationTest, InstantiatingThroughANonRepresentativeBindsTheWholeClass) {
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = var(0)}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 1, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_ubs_state at_grandchild = binder.bind(binder.bind(pud_ubs_state{}, child), grandchild);
    const ubs_class var_class_zero = ubs_bind_map{at_grandchild}.find(0);
    const ubs_class var_class_one = ubs_bind_map{at_grandchild}.find(1);
    if (var_class_zero.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_zero.value.value()), functor("a", {}));
    else
        EXPECT_EQ(exprs.make_var(var_class_zero.rep), functor("a", {}));
    if (var_class_one.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_one.value.value()), functor("a", {}));
    else
        EXPECT_EQ(exprs.make_var(var_class_one.rep), functor("a", {}));
}

TEST_F(PudBinderIntegrationTest, NodeWithNoSpecsKeepsTheParentsBindings) {
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    const pud_global_node_id grandchild = modules.store_inference(k_module, pud_inference{
        .parent = child.node,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {},
        .var_count = 0,
    });
    pud_ubs_state at_grandchild = binder.bind(binder.bind(pud_ubs_state{}, child), grandchild);
    const ubs_class var_class_zero = ubs_bind_map{at_grandchild}.find(0);
    const ubs_class var_class_one = ubs_bind_map{at_grandchild}.find(1);
    if (var_class_zero.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_zero.value.value()), functor("a", {}));
    else
        EXPECT_EQ(exprs.make_var(var_class_zero.rep), functor("a", {}));
    if (var_class_one.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, var_class_one.value.value()), var(1));
    else
        EXPECT_EQ(exprs.make_var(var_class_one.rep), var(1));
}

TEST_F(PudBinderIntegrationTest, ManySpecsAtOneNode) {
    const uint32_t substitution_count = 200;
    std::vector<pud_substitution> substitutions;
    for (uint32_t index = 0; index < substitution_count; ++index) {
        const std::string label = "v" + std::to_string(index);
        substitutions.push_back(pud_substitution{.var_idx = index, .value = functor(label.c_str(), {})});
    }
    const pud_global_node_id child = modules.store_inference(k_module, pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = std::move(substitutions),
        .body_goals = {},
        .var_count = 0,
    });
    pud_ubs_state at_child = binder.bind(pud_ubs_state{}, child);
    for (uint32_t index = 0; index < substitution_count; ++index) {
        const std::string label = "v" + std::to_string(index);
        const ubs_class var_class = ubs_bind_map{at_child}.find(index);
        if (var_class.value.has_value())
            EXPECT_EQ(expr_from_framed_functor(exprs, var_class.value.value()), functor(label.c_str(), {}));
        else
            EXPECT_EQ(exprs.make_var(var_class.rep), functor(label.c_str(), {}));
    }
}

TEST_F(PudBinderIntegrationTest, DeepEquateChainThenOneInstantiation) {
    const uint32_t depth = 200;
    pud_ubs_state at = pud_ubs_state{};
    pud_global_node_id prior_node = pud_global_node_id{.module = k_module, .node = 0};
    for (uint32_t level = 1; level <= depth; ++level) {
        const pud_global_node_id level_node = modules.store_inference(k_module, pud_inference{
            .parent = prior_node.node,
            .call_site = 0,
            .substitutions = {pud_substitution{.var_idx = level, .value = var(level - 1)}},
            .body_goals = {},
            .var_count = 0,
        });
        at = binder.bind(at, level_node);
        prior_node = level_node;
    }
    for (uint32_t index = 0; index <= depth; ++index) {
        const ubs_class var_class = ubs_bind_map{at}.find(index);
        if (var_class.value.has_value())
            EXPECT_EQ(expr_from_framed_functor(exprs, var_class.value.value()), var(0)) << "var " << index;
        else
            EXPECT_EQ(exprs.make_var(var_class.rep), var(0)) << "var " << index;
    }
    const pud_global_node_id bottom = modules.store_inference(k_module, pud_inference{
        .parent = prior_node.node,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = depth, .value = functor("a", {})}},
        .body_goals = {},
        .var_count = 0,
    });
    pud_ubs_state at_bottom = binder.bind(at, bottom);
    for (uint32_t index = 0; index <= depth; ++index) {
        const ubs_class var_class = ubs_bind_map{at_bottom}.find(index);
        if (var_class.value.has_value())
            EXPECT_EQ(expr_from_framed_functor(exprs, var_class.value.value()), functor("a", {})) << "var " << index;
        else
            EXPECT_EQ(exprs.make_var(var_class.rep), functor("a", {})) << "var " << index;
    }
    const ubs_class depth_class = ubs_bind_map{at}.find(depth);
    if (depth_class.value.has_value())
        EXPECT_EQ(expr_from_framed_functor(exprs, depth_class.value.value()), var(0));
    else
        EXPECT_EQ(exprs.make_var(depth_class.rep), var(0));
}
