#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_binding_descent.hpp"
#include "infrastructure/pud_node_added_specializations.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/pud_specialization.hpp"

struct PudBindingDescentIntegrationTest : public ::testing::Test {
    using bind_map_t = ubs_transient_bind_map<expr_pool, expr_pool>;
    using binding_descent_t = pud_binding_descent<
        bind_map_t,
        pud_node_added_specializations,
        expr_pool,
        expr_pool>;

    test_functors                  functors;
    expr_pool                      exprs;
    pud_node_added_specializations node_specs_;
    pud_node_id child = 1;
    pud_node_id grandchild = 2;
    pud_node_id sibling = 3;

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    binding_descent_t at_root() {
        return binding_descent_t{node_specs_, exprs, exprs};
    }

    const expr* resolve(const binding_descent_t& descent, uint32_t var_index) {
        pud_transient_ubs_state transient{descent.ubs_state()};
        bind_map_t bind_map = transient.make_bind_map(exprs, exprs);
        return bind_map.resolve(var_index);
    }
};

TEST_F(PudBindingDescentIntegrationTest, RootHasNoBindings) {
    EXPECT_EQ(resolve(at_root(), 0), var(0));
    EXPECT_EQ(resolve(at_root(), 3), var(3));
}

TEST_F(PudBindingDescentIntegrationTest, FunctorSpecInstantiates) {
    node_specs_.store(child, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    EXPECT_EQ(resolve(at_root().descend(child), 0), functor("a", {}));
}

TEST_F(PudBindingDescentIntegrationTest, VarSpecEquates) {
    node_specs_.store(child, {pud_specialization{.var_idx = 1, .value = var(0)}});
    binding_descent_t at_child = at_root().descend(child);
    EXPECT_EQ(resolve(at_child, 0), var(0));
    EXPECT_EQ(resolve(at_child, 1), var(0));
}

TEST_F(PudBindingDescentIntegrationTest, MultiLevelChainResolves) {
    node_specs_.store(child, {pud_specialization{.var_idx = 1, .value = var(0)}});
    node_specs_.store(grandchild, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    binding_descent_t at_grandchild = at_root().descend(child).descend(grandchild);
    EXPECT_EQ(resolve(at_grandchild, 0), functor("a", {}));
    EXPECT_EQ(resolve(at_grandchild, 1), functor("a", {}));
}

TEST_F(PudBindingDescentIntegrationTest, ParentUnchangedByChild) {
    node_specs_.store(child, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    binding_descent_t root_descent = at_root();
    root_descent.descend(child);
    EXPECT_EQ(resolve(root_descent, 0), var(0));
}

TEST_F(PudBindingDescentIntegrationTest, SiblingsDescendIndependently) {
    node_specs_.store(child, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    node_specs_.store(sibling, {pud_specialization{.var_idx = 0, .value = functor("b", {})}});
    binding_descent_t root_descent = at_root();
    EXPECT_EQ(resolve(root_descent.descend(child), 0), functor("a", {}));
    EXPECT_EQ(resolve(root_descent.descend(sibling), 0), functor("b", {}));
}

TEST_F(PudBindingDescentIntegrationTest, SeveralSpecsAtOneNodeApplyInOrder) {
    node_specs_.store(child, {
        pud_specialization{.var_idx = 1, .value = var(0)},
        pud_specialization{.var_idx = 0, .value = functor("a", {})},
    });
    binding_descent_t at_child = at_root().descend(child);
    EXPECT_EQ(resolve(at_child, 0), functor("a", {}));
    EXPECT_EQ(resolve(at_child, 1), functor("a", {}));
}

TEST_F(PudBindingDescentIntegrationTest, InstantiationWithVarArgsLeavesTheArgsUnbound) {
    node_specs_.store(child, {pud_specialization{.var_idx = 0, .value = functor("f", {var(1), var(2)})}});
    binding_descent_t at_child = at_root().descend(child);
    EXPECT_EQ(resolve(at_child, 0), functor("f", {var(1), var(2)}));
    EXPECT_EQ(resolve(at_child, 1), var(1));
    EXPECT_EQ(resolve(at_child, 2), var(2));
}

TEST_F(PudBindingDescentIntegrationTest, BindingAnArgLaterIsSeenThroughTheFunctor) {
    node_specs_.store(child, {pud_specialization{.var_idx = 0, .value = functor("f", {var(1)})}});
    node_specs_.store(grandchild, {pud_specialization{.var_idx = 1, .value = functor("b", {})}});
    binding_descent_t at_grandchild = at_root().descend(child).descend(grandchild);
    EXPECT_EQ(resolve(at_grandchild, 0), functor("f", {var(1)}));
    EXPECT_EQ(resolve(at_grandchild, 1), functor("b", {}));
}

TEST_F(PudBindingDescentIntegrationTest, EquatedClassKeepsTheSmallestIndexAsRepresentative) {
    node_specs_.store(child, {pud_specialization{.var_idx = 5, .value = var(3)}});
    node_specs_.store(grandchild, {pud_specialization{.var_idx = 3, .value = var(1)}});
    binding_descent_t at_grandchild = at_root().descend(child).descend(grandchild);
    EXPECT_EQ(resolve(at_grandchild, 5), var(1));
    EXPECT_EQ(resolve(at_grandchild, 3), var(1));
    EXPECT_EQ(resolve(at_grandchild, 1), var(1));
}

TEST_F(PudBindingDescentIntegrationTest, InstantiatingThroughANonRepresentativeBindsTheWholeClass) {
    node_specs_.store(child, {pud_specialization{.var_idx = 1, .value = var(0)}});
    node_specs_.store(grandchild, {pud_specialization{.var_idx = 1, .value = functor("a", {})}});
    binding_descent_t at_grandchild = at_root().descend(child).descend(grandchild);
    EXPECT_EQ(resolve(at_grandchild, 0), functor("a", {}));
    EXPECT_EQ(resolve(at_grandchild, 1), functor("a", {}));
}

TEST_F(PudBindingDescentIntegrationTest, NodeWithNoSpecsKeepsTheParentsBindings) {
    node_specs_.store(child, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    node_specs_.store(grandchild, {});
    binding_descent_t at_grandchild = at_root().descend(child).descend(grandchild);
    EXPECT_EQ(resolve(at_grandchild, 0), functor("a", {}));
    EXPECT_EQ(resolve(at_grandchild, 1), var(1));
}

TEST_F(PudBindingDescentIntegrationTest, ManySpecsAtOneNode) {
    const uint32_t spec_count = 200;
    std::vector<pud_specialization> specs;
    for (uint32_t index = 0; index < spec_count; ++index) {
        const std::string label = "v" + std::to_string(index);
        specs.push_back(pud_specialization{.var_idx = index, .value = functor(label.c_str(), {})});
    }
    node_specs_.store(child, std::move(specs));
    binding_descent_t at_child = at_root().descend(child);
    for (uint32_t index = 0; index < spec_count; ++index) {
        const std::string label = "v" + std::to_string(index);
        EXPECT_EQ(resolve(at_child, index), functor(label.c_str(), {}));
    }
}

TEST_F(PudBindingDescentIntegrationTest, DeepEquateChainThenOneInstantiation) {
    const uint32_t depth = 200;
    binding_descent_t at = at_root();
    for (uint32_t level = 1; level <= depth; ++level) {
        const pud_node_id level_node = 1000 + level;
        node_specs_.store(level_node, {pud_specialization{.var_idx = level, .value = var(level - 1)}});
        at = at.descend(level_node);
    }
    for (uint32_t index = 0; index <= depth; ++index)
        EXPECT_EQ(resolve(at, index), var(0)) << "var " << index;
    node_specs_.store(9999, {pud_specialization{.var_idx = depth, .value = functor("a", {})}});
    binding_descent_t at_bottom = at.descend(9999);
    for (uint32_t index = 0; index <= depth; ++index)
        EXPECT_EQ(resolve(at_bottom, index), functor("a", {})) << "var " << index;
    EXPECT_EQ(resolve(at, depth), var(0));
}

TEST_F(PudBindingDescentIntegrationTest, DescentIsAssignable) {
    node_specs_.store(child, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    binding_descent_t current = at_root();
    current = current.descend(child);
    EXPECT_EQ(resolve(current, 0), functor("a", {}));
}
