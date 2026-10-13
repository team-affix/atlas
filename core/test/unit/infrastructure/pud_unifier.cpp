#include <deque>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unifier.hpp"
#include "value_objects/expr.hpp"

namespace {

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t frame_offset, uint32_t local_index), ());
};

struct MockNormalize {
    MOCK_METHOD(const expr*, normalize, (framed_expr));
};

struct MockResolve {
    MOCK_METHOD(const expr*, resolve, (uint32_t));
};

struct MockEquate {
    MOCK_METHOD(void, equate, (uint32_t, uint32_t));
};

struct MockInstantiate {
    MOCK_METHOD(void, instantiate, (uint32_t, const expr*));
};

using test_unifier_t = unifier<
    ::testing::NiceMock<MockGlobalize>,
    ::testing::NiceMock<MockNormalize>,
    ::testing::NiceMock<MockResolve>,
    ::testing::NiceMock<MockEquate>,
    ::testing::NiceMock<MockInstantiate>>;

struct expr_fixture {
    std::deque<expr> owned;

    const expr* var(uint32_t index) {
        owned.push_back(expr{expr::var{index}});
        return &owned.back();
    }

    const expr* functor(uint32_t id, const std::vector<const expr*>& args) {
        owned.push_back(expr{expr::functor{id, args}});
        return &owned.back();
    }
};

} // namespace

struct PudUnifierTest : public ::testing::Test {
    expr_fixture terms;
    ::testing::NiceMock<MockGlobalize> globalize;
    ::testing::NiceMock<MockNormalize> normalize;
    ::testing::NiceMock<MockResolve> resolve;
    ::testing::NiceMock<MockEquate> equate;
    ::testing::NiceMock<MockInstantiate> instantiate;
    test_unifier_t sut{globalize, normalize, resolve, equate, instantiate};

    const expr* var0{terms.var(0u)};
    const expr* var1{terms.var(1u)};
    const expr* var2{terms.var(2u)};
    const expr* atom{terms.functor(10u, {})};
    const expr* f0{terms.functor(1u, {var0})};
    const expr* g0{terms.functor(2u, {})};
    const expr* f2{terms.functor(1u, {var0, var1})};
};

TEST_F(PudUnifierTest, PointerEqualityShortCircuitNeedsNoBindOps) {
    ::testing::StrictMock<MockEquate> strict_equate;
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    using strict_unifier_t = unifier<
        ::testing::NiceMock<MockGlobalize>,
        ::testing::NiceMock<MockNormalize>,
        ::testing::NiceMock<MockResolve>,
        ::testing::StrictMock<MockEquate>,
        ::testing::StrictMock<MockInstantiate>>;
    strict_unifier_t strict_unifier{globalize, normalize, resolve, strict_equate, strict_instantiate};
    EXPECT_TRUE(strict_unifier.unify({atom, 0u}, {atom, 0u}));
}

TEST_F(PudUnifierTest, UnifyTwoVarsEquatesIndices) {
    EXPECT_CALL(globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(globalize, globalize(0u, 1u)).WillOnce(testing::Return(1u));
    EXPECT_CALL(resolve, resolve(0u)).WillOnce(testing::Return(var0));
    EXPECT_CALL(resolve, resolve(1u)).WillOnce(testing::Return(var1));
    EXPECT_CALL(equate, equate(0u, 1u));
    EXPECT_TRUE(sut.unify({var0, 0u}, {var1, 0u}));
}

TEST_F(PudUnifierTest, UnifyVarWithFunctorNormalizesThenInstantiates) {
    EXPECT_CALL(globalize, globalize(0u, 2u)).WillOnce(testing::Return(2u));
    EXPECT_CALL(resolve, resolve(2u)).WillOnce(testing::Return(var2));
    EXPECT_CALL(normalize, normalize(testing::_)).WillOnce(testing::Return(atom));
    EXPECT_CALL(instantiate, instantiate(2u, atom));
    EXPECT_TRUE(sut.unify({var2, 0u}, {atom, 0u}));
}

TEST_F(PudUnifierTest, OccursCheckRejectsBinding) {
    const expr* cyclic = terms.functor(1u, {var0});
    EXPECT_CALL(globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(resolve, resolve(0u)).WillOnce(testing::Return(var0));
    EXPECT_CALL(normalize, normalize(testing::_)).WillOnce(testing::Return(cyclic));
    EXPECT_FALSE(sut.unify({var0, 0u}, {cyclic, 0u}));
}

TEST_F(PudUnifierTest, MismatchedFunctorIdsFail) {
    EXPECT_FALSE(sut.unify({f0, 0u}, {g0, 0u}));
}

TEST_F(PudUnifierTest, MismatchedArityFails) {
    EXPECT_FALSE(sut.unify({f0, 0u}, {f2, 0u}));
}

struct PudUnifierNestedTest : public ::testing::Test {
    expr_fixture terms;
    ::testing::NiceMock<MockGlobalize> globalize;
    ::testing::NiceMock<MockNormalize> normalize;
    ::testing::NiceMock<MockResolve> resolve;
    ::testing::NiceMock<MockEquate> equate;
    ::testing::NiceMock<MockInstantiate> instantiate;
    test_unifier_t sut{globalize, normalize, resolve, equate, instantiate};

    const expr* inner_bad{terms.functor(5u, {terms.var(0u)})};
    const expr* inner_good{terms.functor(6u, {terms.var(0u)})};
    const expr* outer_l{terms.functor(4u, {inner_bad})};
    const expr* outer_r{terms.functor(4u, {inner_good})};
};

TEST_F(PudUnifierNestedTest, DeepMismatchFails) {
    EXPECT_FALSE(sut.unify({outer_l, 0u}, {outer_r, 0u}));
}

TEST_F(PudUnifierTest, VarLeftFunctorRightMirrorsFunctorLeftVarRight) {
    ::testing::NiceMock<MockGlobalize> g_left;
    ::testing::NiceMock<MockGlobalize> g_right;
    ::testing::NiceMock<MockNormalize> n_left;
    ::testing::NiceMock<MockNormalize> n_right;
    ::testing::NiceMock<MockResolve> r_left;
    ::testing::NiceMock<MockResolve> r_right;
    ::testing::NiceMock<MockEquate> e_left;
    ::testing::NiceMock<MockEquate> e_right;
    ::testing::NiceMock<MockInstantiate> i_left;
    ::testing::NiceMock<MockInstantiate> i_right;
    test_unifier_t left_unifier{g_left, n_left, r_left, e_left, i_left};
    test_unifier_t right_unifier{g_right, n_right, r_right, e_right, i_right};

    EXPECT_CALL(g_left, globalize(0u, 2u)).WillOnce(testing::Return(2u));
    EXPECT_CALL(r_left, resolve(2u)).WillOnce(testing::Return(var2));
    EXPECT_CALL(n_left, normalize(testing::_)).WillOnce(testing::Return(atom));
    EXPECT_CALL(i_left, instantiate(2u, atom));

    EXPECT_CALL(g_right, globalize(0u, 2u)).WillOnce(testing::Return(2u));
    EXPECT_CALL(r_right, resolve(2u)).WillOnce(testing::Return(var2));
    EXPECT_CALL(n_right, normalize(testing::_)).WillOnce(testing::Return(atom));
    EXPECT_CALL(i_right, instantiate(2u, atom));

    EXPECT_TRUE(left_unifier.unify({var2, 0u}, {atom, 0u}));
    EXPECT_TRUE(right_unifier.unify({atom, 0u}, {var2, 0u}));
}

TEST_F(PudUnifierTest, VarVarBothOrdersEquateSameIndices) {
    EXPECT_CALL(globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(globalize, globalize(0u, 1u)).WillOnce(testing::Return(1u));
    EXPECT_CALL(resolve, resolve(0u)).WillOnce(testing::Return(var0));
    EXPECT_CALL(resolve, resolve(1u)).WillOnce(testing::Return(var1));
    EXPECT_CALL(equate, equate(0u, 1u));
    EXPECT_TRUE(sut.unify({var0, 0u}, {var1, 0u}));

    ::testing::NiceMock<MockEquate> equate_ba;
    test_unifier_t reverse{globalize, normalize, resolve, equate_ba, instantiate};
    EXPECT_CALL(globalize, globalize(0u, 1u)).WillOnce(testing::Return(1u));
    EXPECT_CALL(globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(resolve, resolve(1u)).WillOnce(testing::Return(var1));
    EXPECT_CALL(resolve, resolve(0u)).WillOnce(testing::Return(var0));
    EXPECT_CALL(equate_ba, equate(1u, 0u));
    EXPECT_TRUE(reverse.unify({var1, 0u}, {var0, 0u}));
}

using strict_unifier_t = unifier<
    ::testing::StrictMock<MockGlobalize>,
    ::testing::StrictMock<MockNormalize>,
    ::testing::StrictMock<MockResolve>,
    ::testing::StrictMock<MockEquate>,
    ::testing::StrictMock<MockInstantiate>>;

TEST_F(PudUnifierTest, FunctorMismatchAtFirstArgPerformsNoBindOps) {
    ::testing::StrictMock<MockGlobalize> strict_globalize;
    ::testing::StrictMock<MockNormalize> strict_normalize;
    ::testing::StrictMock<MockResolve> strict_resolve;
    ::testing::StrictMock<MockEquate> strict_equate;
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    strict_unifier_t strict{
        strict_globalize, strict_normalize, strict_resolve, strict_equate, strict_instantiate};
    EXPECT_FALSE(strict.unify({f0, 0u}, {g0, 0u}));
}

TEST_F(PudUnifierTest, MismatchedArityPerformsNoBindOps) {
    ::testing::StrictMock<MockGlobalize> strict_globalize;
    ::testing::StrictMock<MockNormalize> strict_normalize;
    ::testing::StrictMock<MockResolve> strict_resolve;
    ::testing::StrictMock<MockEquate> strict_equate;
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    strict_unifier_t strict{
        strict_globalize, strict_normalize, strict_resolve, strict_equate, strict_instantiate};
    EXPECT_FALSE(strict.unify({f0, 0u}, {f2, 0u}));
}

TEST_F(PudUnifierTest, ZeroAritySameIdSucceedsWithoutBindOps) {
    ::testing::StrictMock<MockGlobalize> strict_globalize;
    ::testing::StrictMock<MockNormalize> strict_normalize;
    ::testing::StrictMock<MockResolve> strict_resolve;
    ::testing::StrictMock<MockEquate> strict_equate;
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    strict_unifier_t strict{
        strict_globalize, strict_normalize, strict_resolve, strict_equate, strict_instantiate};
    EXPECT_TRUE(strict.unify({atom, 0u}, {atom, 0u}));
}

TEST_F(PudUnifierTest, ZeroArityDifferentIdFailsWithoutBindOps) {
    const expr* other = terms.functor(11u, {});
    ::testing::StrictMock<MockGlobalize> strict_globalize;
    ::testing::StrictMock<MockNormalize> strict_normalize;
    ::testing::StrictMock<MockResolve> strict_resolve;
    ::testing::StrictMock<MockEquate> strict_equate;
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    strict_unifier_t strict{
        strict_globalize, strict_normalize, strict_resolve, strict_equate, strict_instantiate};
    EXPECT_FALSE(strict.unify({atom, 0u}, {other, 0u}));
}

TEST_F(PudUnifierTest, BoundVarIdenticalFunctorSucceedsWithoutInstantiate) {
    const expr* bound = terms.functor(10u, {});
    ::testing::StrictMock<MockGlobalize> strict_globalize;
    ::testing::StrictMock<MockNormalize> strict_normalize;
    ::testing::StrictMock<MockResolve> strict_resolve;
    ::testing::StrictMock<MockEquate> strict_equate;
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    strict_unifier_t strict{
        strict_globalize, strict_normalize, strict_resolve, strict_equate, strict_instantiate};
    EXPECT_CALL(strict_globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(strict_resolve, resolve(0u)).WillOnce(testing::Return(bound));
    EXPECT_TRUE(strict.unify({var0, 0u}, {bound, 0u}));
}

TEST_F(PudUnifierTest, TwoVarsBoundToSamePointerShortCircuits) {
    const expr* bound = terms.functor(10u, {});
    ::testing::StrictMock<MockGlobalize> strict_globalize;
    ::testing::StrictMock<MockNormalize> strict_normalize;
    ::testing::StrictMock<MockResolve> strict_resolve;
    ::testing::StrictMock<MockEquate> strict_equate;
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    strict_unifier_t strict{
        strict_globalize, strict_normalize, strict_resolve, strict_equate, strict_instantiate};
    EXPECT_CALL(strict_globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(strict_globalize, globalize(0u, 1u)).WillOnce(testing::Return(1u));
    EXPECT_CALL(strict_resolve, resolve(0u)).WillOnce(testing::Return(bound));
    EXPECT_CALL(strict_resolve, resolve(1u)).WillOnce(testing::Return(bound));
    EXPECT_TRUE(strict.unify({var0, 0u}, {var1, 0u}));
}

TEST_F(PudUnifierTest, OccursDeepCycleRejectsBinding) {
    const expr* h = terms.functor(3u, {terms.var(0u)});
    const expr* g = terms.functor(2u, {h});
    const expr* f = terms.functor(1u, {g});
    ::testing::StrictMock<MockGlobalize> strict_globalize;
    ::testing::StrictMock<MockNormalize> strict_normalize;
    ::testing::StrictMock<MockResolve> strict_resolve;
    ::testing::StrictMock<MockEquate> strict_equate;
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    strict_unifier_t strict{
        strict_globalize, strict_normalize, strict_resolve, strict_equate, strict_instantiate};
    EXPECT_CALL(strict_globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(strict_resolve, resolve(0u)).WillOnce(testing::Return(var0));
    EXPECT_CALL(strict_normalize, normalize(testing::_)).WillOnce(testing::Return(f));
    EXPECT_FALSE(strict.unify({var0, 0u}, {f, 0u}));
}

TEST_F(PudUnifierTest, FailedOccursPerformsNoInstantiate) {
    const expr* cyclic = terms.functor(1u, {var0});
    ::testing::StrictMock<MockGlobalize> strict_globalize;
    ::testing::StrictMock<MockNormalize> strict_normalize;
    ::testing::StrictMock<MockResolve> strict_resolve;
    ::testing::StrictMock<MockEquate> strict_equate;
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    strict_unifier_t strict{
        strict_globalize, strict_normalize, strict_resolve, strict_equate, strict_instantiate};
    EXPECT_CALL(strict_globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(strict_resolve, resolve(0u)).WillOnce(testing::Return(var0));
    EXPECT_CALL(strict_normalize, normalize(testing::_)).WillOnce(testing::Return(cyclic));
    EXPECT_FALSE(strict.unify({var0, 0u}, {cyclic, 0u}));
}

TEST_F(PudUnifierTest, AfterFailedOccursLegalBindingStillWorks) {
    const expr* cyclic = terms.functor(1u, {var0});
    EXPECT_CALL(globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(resolve, resolve(0u)).WillOnce(testing::Return(var0));
    EXPECT_CALL(normalize, normalize(testing::_)).WillOnce(testing::Return(cyclic));
    EXPECT_FALSE(sut.unify({var0, 0u}, {cyclic, 0u}));

    EXPECT_CALL(globalize, globalize(0u, 0u)).WillOnce(testing::Return(0u));
    EXPECT_CALL(resolve, resolve(0u)).WillOnce(testing::Return(var0));
    EXPECT_CALL(normalize, normalize(testing::_)).WillOnce(testing::Return(atom));
    EXPECT_CALL(instantiate, instantiate(0u, atom));
    EXPECT_TRUE(sut.unify({var0, 0u}, {atom, 0u}));
}
