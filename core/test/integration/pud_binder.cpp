#include <optional>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_data.hpp"
#include "infrastructure/pud_modules.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "infrastructure/ubs_bind_map.hpp"

// The binder over the real modules and real bind maps. Every inference hangs under the one dummy
// axiom of the test's module.
struct PudBinderIntegrationTest : public ::testing::Test {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;
    using modules_t = pud_modules<pud_data>;

    static constexpr pud_module_id k_module = 0;

    test_functors functors;
    expr_pool exprs;
    globalizer globalize;
    term_factory_t terms{globalize, exprs, exprs};
    pud_data data{k_module, {}, {rule(exprs.make_functor(functors.id("dummy"), {}), {}, 0)}};
    modules_t modules;
    pud_binder<term_factory_t, modules_t> binder{terms, modules};

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    void SetUp() override {
        modules.register_module(data);
    }

    pud_global_node_id infer(std::vector<pud_substitution> substitutions) {
        return modules.store_inference(k_module, pud_node_delta{
            .parent = 0,
            .call_site = 0,
            .substitutions = std::move(substitutions),
            .body_goals = {},
            .var_count = 0,
        });
    }

    const expr* resolve(const pud_ubs_state& bindings, uint32_t var_index) {
        const ubs_class var_class = ubs_bind_map{bindings}.find(var_index);
        if (var_class.value.has_value())
            return var_class.value->skeleton;
        return exprs.make_var(var_class.rep);
    }
};

TEST_F(PudBinderIntegrationTest, RootHasNoBindings) {
    EXPECT_EQ(resolve(pud_ubs_state{}, 0), var(0));
    EXPECT_EQ(resolve(pud_ubs_state{}, 3), var(3));
}

TEST_F(PudBinderIntegrationTest, FunctorSpecInstantiates) {
    const pud_global_node_id child = infer({pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    EXPECT_EQ(resolve(binder.bind(pud_ubs_state{}, child), 0), functor("a", {}));
}

TEST_F(PudBinderIntegrationTest, VarSpecEquates) {
    const pud_global_node_id child = infer({pud_substitution{.var_idx = 1, .value = var(0)}});
    pud_ubs_state at_child = binder.bind(pud_ubs_state{}, child);
    EXPECT_EQ(resolve(at_child, 0), var(0));
    EXPECT_EQ(resolve(at_child, 1), var(0));
}

TEST_F(PudBinderIntegrationTest, MultiLevelChainResolves) {
    const pud_global_node_id child = infer({pud_substitution{.var_idx = 1, .value = var(0)}});
    const pud_global_node_id grandchild = infer({pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    pud_ubs_state at_grandchild = binder.bind(binder.bind(pud_ubs_state{}, child), grandchild);
    EXPECT_EQ(resolve(at_grandchild, 0), functor("a", {}));
    EXPECT_EQ(resolve(at_grandchild, 1), functor("a", {}));
}

TEST_F(PudBinderIntegrationTest, ParentUnchangedByChild) {
    const pud_global_node_id child = infer({pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    pud_ubs_state root_bindings = pud_ubs_state{};
    binder.bind(root_bindings, child);
    EXPECT_EQ(resolve(root_bindings, 0), var(0));
}

TEST_F(PudBinderIntegrationTest, SiblingsDescendIndependently) {
    const pud_global_node_id child = infer({pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    const pud_global_node_id sibling = infer({pud_substitution{.var_idx = 0, .value = functor("b", {})}});
    pud_ubs_state root_bindings = pud_ubs_state{};
    EXPECT_EQ(resolve(binder.bind(root_bindings, child), 0), functor("a", {}));
    EXPECT_EQ(resolve(binder.bind(root_bindings, sibling), 0), functor("b", {}));
}

TEST_F(PudBinderIntegrationTest, SeveralSpecsAtOneNodeApplyInOrder) {
    const pud_global_node_id child = infer({
        pud_substitution{.var_idx = 1, .value = var(0)},
        pud_substitution{.var_idx = 0, .value = functor("a", {})},
    });
    pud_ubs_state at_child = binder.bind(pud_ubs_state{}, child);
    EXPECT_EQ(resolve(at_child, 0), functor("a", {}));
    EXPECT_EQ(resolve(at_child, 1), functor("a", {}));
}

TEST_F(PudBinderIntegrationTest, InstantiationWithVarArgsLeavesTheArgsUnbound) {
    const pud_global_node_id child = infer({pud_substitution{.var_idx = 0, .value = functor("f", {var(1), var(2)})}});
    pud_ubs_state at_child = binder.bind(pud_ubs_state{}, child);
    EXPECT_EQ(resolve(at_child, 0), functor("f", {var(1), var(2)}));
    EXPECT_EQ(resolve(at_child, 1), var(1));
    EXPECT_EQ(resolve(at_child, 2), var(2));
}

TEST_F(PudBinderIntegrationTest, BindingAnArgLaterIsSeenThroughTheFunctor) {
    const pud_global_node_id child = infer({pud_substitution{.var_idx = 0, .value = functor("f", {var(1)})}});
    const pud_global_node_id grandchild = infer({pud_substitution{.var_idx = 1, .value = functor("b", {})}});
    pud_ubs_state at_grandchild = binder.bind(binder.bind(pud_ubs_state{}, child), grandchild);
    EXPECT_EQ(resolve(at_grandchild, 0), functor("f", {var(1)}));
    EXPECT_EQ(resolve(at_grandchild, 1), functor("b", {}));
}

TEST_F(PudBinderIntegrationTest, EquatedClassKeepsTheSmallestIndexAsRepresentative) {
    const pud_global_node_id child = infer({pud_substitution{.var_idx = 5, .value = var(3)}});
    const pud_global_node_id grandchild = infer({pud_substitution{.var_idx = 3, .value = var(1)}});
    pud_ubs_state at_grandchild = binder.bind(binder.bind(pud_ubs_state{}, child), grandchild);
    EXPECT_EQ(resolve(at_grandchild, 5), var(1));
    EXPECT_EQ(resolve(at_grandchild, 3), var(1));
    EXPECT_EQ(resolve(at_grandchild, 1), var(1));
}

TEST_F(PudBinderIntegrationTest, InstantiatingThroughANonRepresentativeBindsTheWholeClass) {
    const pud_global_node_id child = infer({pud_substitution{.var_idx = 1, .value = var(0)}});
    const pud_global_node_id grandchild = infer({pud_substitution{.var_idx = 1, .value = functor("a", {})}});
    pud_ubs_state at_grandchild = binder.bind(binder.bind(pud_ubs_state{}, child), grandchild);
    EXPECT_EQ(resolve(at_grandchild, 0), functor("a", {}));
    EXPECT_EQ(resolve(at_grandchild, 1), functor("a", {}));
}

TEST_F(PudBinderIntegrationTest, NodeWithNoSpecsKeepsTheParentsBindings) {
    const pud_global_node_id child = infer({pud_substitution{.var_idx = 0, .value = functor("a", {})}});
    const pud_global_node_id grandchild = infer({});
    pud_ubs_state at_grandchild = binder.bind(binder.bind(pud_ubs_state{}, child), grandchild);
    EXPECT_EQ(resolve(at_grandchild, 0), functor("a", {}));
    EXPECT_EQ(resolve(at_grandchild, 1), var(1));
}

TEST_F(PudBinderIntegrationTest, ManySpecsAtOneNode) {
    const uint32_t substitution_count = 200;
    std::vector<pud_substitution> substitutions;
    for (uint32_t index = 0; index < substitution_count; ++index) {
        const std::string label = "v" + std::to_string(index);
        substitutions.push_back(pud_substitution{.var_idx = index, .value = functor(label.c_str(), {})});
    }
    const pud_global_node_id child = infer(std::move(substitutions));
    pud_ubs_state at_child = binder.bind(pud_ubs_state{}, child);
    for (uint32_t index = 0; index < substitution_count; ++index) {
        const std::string label = "v" + std::to_string(index);
        EXPECT_EQ(resolve(at_child, index), functor(label.c_str(), {}));
    }
}

TEST_F(PudBinderIntegrationTest, DeepEquateChainThenOneInstantiation) {
    const uint32_t depth = 200;
    pud_ubs_state at = pud_ubs_state{};
    for (uint32_t level = 1; level <= depth; ++level) {
        const pud_global_node_id level_node = infer({pud_substitution{.var_idx = level, .value = var(level - 1)}});
        at = binder.bind(at, level_node);
    }
    for (uint32_t index = 0; index <= depth; ++index)
        EXPECT_EQ(resolve(at, index), var(0)) << "var " << index;
    const pud_global_node_id bottom = infer({pud_substitution{.var_idx = depth, .value = functor("a", {})}});
    pud_ubs_state at_bottom = binder.bind(at, bottom);
    for (uint32_t index = 0; index <= depth; ++index)
        EXPECT_EQ(resolve(at_bottom, index), functor("a", {})) << "var " << index;
    EXPECT_EQ(resolve(at, depth), var(0));
}
