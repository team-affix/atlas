#include <cstdint>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_solution_normalizer.hpp"
#include "infrastructure/ubs_bind_map.hpp"

using ::testing::Return;

namespace {

struct MockGetGlobalFunctorId {
    MOCK_METHOD(uint32_t, get_global_functor_id_from_local_functor_id, (pud_module_id, uint32_t));
};

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t));
};

struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor, (uint32_t, (const std::vector<const expr*>&)));
};

struct PudSolutionNormalizerTest : public ::testing::Test {
    MockGetGlobalFunctorId globals;
    MockMakeVar vars;
    MockMakeFunctor functors;
    pud_solution_normalizer<MockGetGlobalFunctorId, MockMakeVar, MockMakeFunctor> normalizer{
        globals, vars, functors};
    expr variable{expr::var{0}};
    expr inner_variable{expr::var{1}};
    expr foreign_atom{expr::functor{8, {&inner_variable}}};
    expr rebuilt_var{expr::var{7}};
    expr rebuilt_inner{expr::var{1}};
    expr rebuilt_atom{expr::functor{20, {}}};
    expr rebuilt_outer{expr::functor{21, {&rebuilt_inner}}};

    pud_ubs_state bound_to_foreign_atom() {
        ubs_bind_map bindings{pud_ubs_state{}};
        bindings.instantiate(
            bindings.find(0),
            moduled_functor{std::get_if<expr::functor>(&foreign_atom.content), 0, 4});
        return std::move(bindings).persist();
    }
};

}

TEST_F(PudSolutionNormalizerTest, AnUnboundVariableBecomesItsRepresentative) {
    EXPECT_CALL(vars, make_var(0u)).WillOnce(Return(&rebuilt_var));
    EXPECT_EQ(normalizer.normalize(&variable, 1, pud_ubs_state{}), &rebuilt_var);
}

TEST_F(PudSolutionNormalizerTest, ABoundVariableUsesTheBindingsModule) {
    const pud_ubs_state state = bound_to_foreign_atom();
    EXPECT_CALL(vars, make_var(1u)).WillOnce(Return(&rebuilt_inner));
    EXPECT_CALL(globals, get_global_functor_id_from_local_functor_id(4u, 8u)).WillOnce(Return(20u));
    EXPECT_CALL(functors, make_functor(20u, std::vector<const expr*>{&rebuilt_inner})).WillOnce(Return(&rebuilt_atom));
    EXPECT_EQ(normalizer.normalize(&variable, 1, state), &rebuilt_atom);
}

TEST_F(PudSolutionNormalizerTest, AFunctorTranslatesItsOwnIdAfterItsArguments) {
    expr outer{expr::functor{5, {&variable}}};
    EXPECT_CALL(vars, make_var(0u)).WillOnce(Return(&rebuilt_var));
    EXPECT_CALL(globals, get_global_functor_id_from_local_functor_id(1u, 5u)).WillOnce(Return(21u));
    EXPECT_CALL(functors, make_functor(21u, std::vector<const expr*>{&rebuilt_var})).WillOnce(Return(&rebuilt_outer));
    EXPECT_EQ(normalizer.normalize(&outer, 1, pud_ubs_state{}), &rebuilt_outer);
}
