#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/globalizer.hpp"
#include "infrastructure/ubs_unifier.hpp"
#include "value_objects/expr.hpp"

namespace {

struct MockGlobalize {
    MOCK_METHOD(uint32_t, globalize, (uint32_t frame_offset, uint32_t local_index), ());
};

struct MockResolve {
    MOCK_METHOD(framed_expr, resolve, (uint32_t));
};

struct MockEquate {
    MOCK_METHOD(void, equate, (uint32_t, uint32_t));
};

struct MockInstantiate {
    MOCK_METHOD(void, instantiate, (uint32_t, framed_expr));
};

using test_ubs_unifier_t = ubs_unifier<
    ::testing::NiceMock<MockGlobalize>,
    ::testing::NiceMock<MockResolve>,
    ::testing::NiceMock<MockEquate>,
    ::testing::NiceMock<MockInstantiate>>;

} // namespace

struct UbsUnifierTest : public ::testing::Test {
    ::testing::NiceMock<MockGlobalize> globalize;
    ::testing::NiceMock<MockResolve> resolve;
    ::testing::NiceMock<MockEquate> equate;
    ::testing::NiceMock<MockInstantiate> instantiate;
    test_ubs_unifier_t unifier{globalize, resolve, equate, instantiate};

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr var3{expr::var{3}};
    expr func{expr::functor{7u, {}}};
    expr nilary_a{expr::functor{10u, {}}};
    expr nilary_b{expr::functor{11u, {}}};
    expr f0{expr::functor{1u, {&var0}}};
    expr g0{expr::functor{2u, {}}};
    expr f2_var0{expr::functor{1u, {&var0, &var1}}};
    expr f2_var2{expr::functor{1u, {&var2, &var3}}};
    expr f_xx{expr::functor{1u, {&var0, &var0}}};
    expr atom_a{expr::functor{10u, {}}};
    expr atom_b{expr::functor{11u, {}}};
    expr f_ab{expr::functor{1u, {&atom_a, &atom_b}}};
    expr f_aa{expr::functor{1u, {&atom_a, &atom_a}}};
    expr f_xy{expr::functor{1u, {&var0, &var1}}};
    expr f_y_a{expr::functor{1u, {&var1, &atom_a}}};
};

struct UbsUnifierNestedTest : public ::testing::Test {
    ::testing::NiceMock<MockGlobalize> globalize;
    ::testing::NiceMock<MockResolve> resolve;
    ::testing::NiceMock<MockEquate> equate;
    ::testing::NiceMock<MockInstantiate> instantiate;
    test_ubs_unifier_t unifier{globalize, resolve, equate, instantiate};

    expr var0{expr::var{0}};
    expr inner_l{expr::functor{5u, {&var0}}};
    expr inner_r{expr::functor{6u, {&var0}}};
    expr outer_l{expr::functor{4u, {&inner_l}}};
    expr outer_r{expr::functor{4u, {&inner_r}}};
};

TEST_F(UbsUnifierTest, UnifyTwoVarsEquatesGlobalKeys) {
    EXPECT_CALL(globalize, globalize(0u, 0u)).Times(2).WillRepeatedly(testing::Return(0u));
    EXPECT_CALL(globalize, globalize(0u, 1u)).Times(2).WillRepeatedly(testing::Return(1u));
    EXPECT_CALL(resolve, resolve(0u)).WillOnce(testing::Return(framed_expr{&var0, 0}));
    EXPECT_CALL(resolve, resolve(1u)).WillOnce(testing::Return(framed_expr{&var1, 0}));
    EXPECT_CALL(equate, equate(0u, 1u));
    EXPECT_TRUE(unifier.unify({&var0, 0}, {&var1, 0}));
}

TEST_F(UbsUnifierTest, UnifyVarWithFunctorInstantiatesAtGlobalKey) {
    EXPECT_CALL(globalize, globalize(0u, 2u)).Times(2).WillRepeatedly(testing::Return(2u));
    EXPECT_CALL(resolve, resolve(2u)).WillOnce(testing::Return(framed_expr{&var2, 0}));
    EXPECT_CALL(instantiate, instantiate(2u, framed_expr{&func, 0}));
    EXPECT_TRUE(unifier.unify({&var2, 0}, {&func, 0}));
}

TEST_F(UbsUnifierTest, UnifyFunctorWithVarInstantiatesVarSide) {
    EXPECT_CALL(globalize, globalize(0u, 2u)).Times(2).WillRepeatedly(testing::Return(2u));
    EXPECT_CALL(resolve, resolve(2u)).WillOnce(testing::Return(framed_expr{&var2, 0}));
    EXPECT_CALL(instantiate, instantiate(2u, framed_expr{&func, 0}));
    EXPECT_TRUE(unifier.unify({&func, 0}, {&var2, 0}));
}

TEST_F(UbsUnifierTest, UnifyIdenticalNilaryFunctorsSucceeds) {
    EXPECT_TRUE(unifier.unify({&nilary_a, 0}, {&nilary_a, 0}));
}

TEST_F(UbsUnifierTest, UnifyDifferentFunctorIdsFails) {
    EXPECT_FALSE(unifier.unify({&f0, 0}, {&g0, 0}));
}

TEST_F(UbsUnifierTest, UnifySameIdDifferentArityFails) {
    EXPECT_FALSE(unifier.unify({&f0, 0}, {&f2_var0, 0}));
}

TEST_F(UbsUnifierTest, UnifySameShapeFunctorsRecursesIntoArgs) {
    EXPECT_CALL(globalize, globalize(0u, 0u)).Times(2).WillRepeatedly(testing::Return(0u));
    EXPECT_CALL(globalize, globalize(0u, 2u)).Times(2).WillRepeatedly(testing::Return(2u));
    EXPECT_CALL(globalize, globalize(0u, 1u)).Times(2).WillRepeatedly(testing::Return(1u));
    EXPECT_CALL(globalize, globalize(0u, 3u)).Times(2).WillRepeatedly(testing::Return(3u));
    EXPECT_CALL(resolve, resolve(0u)).WillOnce(testing::Return(framed_expr{&var0, 0}));
    EXPECT_CALL(resolve, resolve(2u)).WillOnce(testing::Return(framed_expr{&var2, 0}));
    EXPECT_CALL(resolve, resolve(1u)).WillOnce(testing::Return(framed_expr{&var1, 0}));
    EXPECT_CALL(resolve, resolve(3u)).WillOnce(testing::Return(framed_expr{&var3, 0}));
    EXPECT_CALL(equate, equate(0u, 2u));
    EXPECT_CALL(equate, equate(1u, 3u));
    EXPECT_TRUE(unifier.unify({&f2_var0, 0}, {&f2_var2, 0}));
}

TEST_F(UbsUnifierNestedTest, UnifyMismatchDeepInNestedArgsFails) {
    EXPECT_FALSE(unifier.unify({&outer_l, 0}, {&outer_r, 0}));
}

TEST_F(UbsUnifierTest, UnifyRepeatedVarFailsWhenSecondArgConflicts) {
    EXPECT_CALL(globalize, globalize(0u, 0u)).Times(3).WillRepeatedly(testing::Return(0u));
    EXPECT_CALL(resolve, resolve(0u))
        .WillOnce(testing::Return(framed_expr{&var0, 0}))
        .WillOnce(testing::Return(framed_expr{&atom_a, 0}));
    EXPECT_CALL(instantiate, instantiate(0u, framed_expr{&atom_a, 0}));
    EXPECT_FALSE(unifier.unify({&f_xx, 0}, {&f_ab, 0}));
}

TEST_F(UbsUnifierTest, UnifyRepeatedVarSucceedsWhenArgsMatch) {
    EXPECT_CALL(globalize, globalize(0u, 0u)).Times(3).WillRepeatedly(testing::Return(0u));
    EXPECT_CALL(resolve, resolve(0u))
        .WillOnce(testing::Return(framed_expr{&var0, 0}))
        .WillOnce(testing::Return(framed_expr{&atom_a, 0}));
    EXPECT_CALL(instantiate, instantiate(0u, framed_expr{&atom_a, 0}));
    EXPECT_TRUE(unifier.unify({&f_xx, 0}, {&f_aa, 0}));
}

TEST_F(UbsUnifierTest, UnifyBoundFunctorsSameStructureDoesNotEquate) {
    ::testing::StrictMock<MockEquate> strict_equate;
    ubs_unifier<
        ::testing::NiceMock<MockGlobalize>,
        ::testing::NiceMock<MockResolve>,
        ::testing::StrictMock<MockEquate>,
        ::testing::NiceMock<MockInstantiate>> strict_unifier{globalize, resolve, strict_equate, instantiate};
    EXPECT_CALL(globalize, globalize(0u, 0u)).Times(2).WillRepeatedly(testing::Return(5u));
    EXPECT_CALL(resolve, resolve(5u))
        .WillOnce(testing::Return(framed_expr{&atom_a, 0}))
        .WillOnce(testing::Return(framed_expr{&atom_a, 0}));
    EXPECT_TRUE(strict_unifier.unify({&var0, 0}, {&var0, 0}));
}
