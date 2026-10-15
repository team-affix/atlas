#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_pool_ubs_factories.hpp"
#include "infrastructure/pud_binding_descent.hpp"
#include "infrastructure/pud_node_added_specializations.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/pud_specialization.hpp"

struct PudBindingDescentIntegrationTest : public ::testing::Test {
    using bind_map_t = ubs_transient_bind_map<expr_pool_ubs_var_factory, expr_pool_ubs_functor_factory>;
    using binding_descent_t = pud_binding_descent<
        bind_map_t,
        pud_node_added_specializations,
        expr_pool_ubs_var_factory,
        expr_pool_ubs_functor_factory>;

    test_functors                  functors;
    expr_pool                      exprs;
    expr_pool_ubs_var_factory      ubs_var_factory_{exprs};
    expr_pool_ubs_functor_factory  ubs_functor_factory_{exprs};
    pud_node_added_specializations node_specs_;
    pud_node_id child = 1;
    pud_node_id grandchild = 2;
    pud_node_id sibling = 3;

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    binding_descent_t at_root() {
        return binding_descent_t{node_specs_, ubs_var_factory_, ubs_functor_factory_};
    }

    const expr* resolve(const binding_descent_t& descent, uint32_t var_index) {
        pud_transient_ubs_state transient{descent.ubs_state()};
        bind_map_t bind_map = transient.make_bind_map(ubs_var_factory_, ubs_functor_factory_);
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

TEST_F(PudBindingDescentIntegrationTest, DescentIsAssignable) {
    node_specs_.store(child, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    binding_descent_t current = at_root();
    current = current.descend(child);
    EXPECT_EQ(resolve(current, 0), functor("a", {}));
}
