#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "reference_pud_normalize_with_bindings.hpp"
#include "infrastructure/expr_pool.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_ubs_state.hpp"

struct ReferencePudNormalizeWithBindingsTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }
};

TEST_F(ReferencePudNormalizeWithBindingsTest, GroundFunctorIsUnchanged) {
    ubs_bind_map bind_map{pud_ubs_state{}};
    EXPECT_EQ(reference_pud_normalize_with_bindings(bind_map, exprs, atom("a")), atom("a"));
}

TEST_F(ReferencePudNormalizeWithBindingsTest, UnboundVariableBecomesItsRepresentative) {
    ubs_bind_map bind_map{pud_ubs_state{}};
    EXPECT_EQ(reference_pud_normalize_with_bindings(bind_map, exprs, var(3)), var(3));
}

TEST_F(ReferencePudNormalizeWithBindingsTest, ChainedBindingFollowsToTheValue) {
    ubs_bind_map bind_map{pud_ubs_state{}};
    bind_map.equate(bind_map.find(0), bind_map.find(1));
    bind_map.instantiate(bind_map.find(1), framed_functor{std::get_if<expr::functor>(&(atom("a"))->content), 0});
    EXPECT_EQ(reference_pud_normalize_with_bindings(bind_map, exprs, var(0)), atom("a"));
}

TEST_F(ReferencePudNormalizeWithBindingsTest, NestedFunctorNormalizesArguments) {
    ubs_bind_map bind_map{pud_ubs_state{}};
    bind_map.instantiate(bind_map.find(0), framed_functor{std::get_if<expr::functor>(&(atom("a"))->content), 0});
    const expr* skeleton = functor("f", {var(0), atom("b")});
    EXPECT_EQ(reference_pud_normalize_with_bindings(bind_map, exprs, skeleton), functor("f", {atom("a"), atom("b")}));
}
