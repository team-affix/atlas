#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_pool_ubs_factories.hpp"
#include "infrastructure/pud_binding_descender.hpp"
#include "infrastructure/pud_node_added_specializations.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/pud_specialization.hpp"

struct PudBindingDescenderIntegrationTest : public ::testing::Test {
    using bind_map_t = ubs_transient_bind_map<expr_pool_ubs_var_factory, expr_pool_ubs_functor_factory>;
    using descender_t = pud_binding_descender<
        bind_map_t,
        pud_node_added_specializations,
        expr_pool_ubs_var_factory,
        expr_pool_ubs_functor_factory>;

    test_functors                 functors;
    expr_pool                     exprs;
    expr_pool_ubs_var_factory     ubs_var_factory_{exprs};
    expr_pool_ubs_functor_factory ubs_functor_factory_{exprs};
    pud_node_added_specializations node_specs_;
    descender_t                   descender_{node_specs_, ubs_var_factory_, ubs_functor_factory_};

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    const expr* resolve(const pud_binding_descent& descent, uint32_t var_index) {
        pud_transient_ubs_state transient{descent.ubs_state};
        bind_map_t bind_map = transient.make_bind_map(ubs_var_factory_, ubs_functor_factory_);
        return bind_map.resolve(var_index);
    }
};

TEST_F(PudBindingDescenderIntegrationTest, FunctorSpecInstantiates) {
    const pud_node_id root = 0;
    const pud_node_id child = 1;
    node_specs_.store(child, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    pud_binding_descent at_root = descender_.root_descent(root);
    pud_binding_descent at_child = descender_.descend(at_root, child);
    EXPECT_EQ(resolve(at_child, 0), functor("a", {}));
}

TEST_F(PudBindingDescenderIntegrationTest, VarSpecEquates) {
    const pud_node_id root = 0;
    const pud_node_id child = 1;
    node_specs_.store(child, {pud_specialization{.var_idx = 1, .value = var(0)}});
    pud_binding_descent at_root = descender_.root_descent(root);
    pud_binding_descent at_child = descender_.descend(at_root, child);
    EXPECT_EQ(resolve(at_child, 0), var(0));
    EXPECT_EQ(resolve(at_child, 1), var(0));
}

TEST_F(PudBindingDescenderIntegrationTest, MultiLevelChainResolves) {
    const pud_node_id root = 0;
    const pud_node_id mid = 1;
    const pud_node_id leaf = 2;
    node_specs_.store(mid, {pud_specialization{.var_idx = 1, .value = var(0)}});
    node_specs_.store(leaf, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    pud_binding_descent at_root = descender_.root_descent(root);
    pud_binding_descent at_mid = descender_.descend(at_root, mid);
    pud_binding_descent at_leaf = descender_.descend(at_mid, leaf);
    EXPECT_EQ(resolve(at_leaf, 0), functor("a", {}));
    EXPECT_EQ(resolve(at_leaf, 1), functor("a", {}));
}

TEST_F(PudBindingDescenderIntegrationTest, ParentDescentUnchangedByChild) {
    const pud_node_id root = 0;
    const pud_node_id child = 1;
    node_specs_.store(child, {pud_specialization{.var_idx = 0, .value = functor("a", {})}});
    pud_binding_descent at_root = descender_.root_descent(root);
    pud_binding_descent at_root_copy = at_root;
    descender_.descend(at_root, child);
    EXPECT_EQ(at_root.ubs_state, at_root_copy.ubs_state);
}
