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

using unifier_no_instantiate_t = unifier<
    ::testing::NiceMock<MockGlobalize>,
    ::testing::NiceMock<MockNormalize>,
    ::testing::NiceMock<MockResolve>,
    ::testing::NiceMock<MockEquate>,
    ::testing::StrictMock<MockInstantiate>>;

using unifier_no_bind_ops_t = unifier<
    ::testing::NiceMock<MockGlobalize>,
    ::testing::NiceMock<MockNormalize>,
    ::testing::NiceMock<MockResolve>,
    ::testing::StrictMock<MockEquate>,
    ::testing::StrictMock<MockInstantiate>>;

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

struct frame_plus_local_globalize {
    uint32_t globalize(uint32_t frame_offset, uint32_t local_index) {
        return frame_offset + local_index;
    }
};

struct global_index_var_resolve {
    expr_fixture& terms_;

    const expr* resolve(uint32_t global_index) {
        return terms_.var(global_index);
    }
};

void stub_globalize_and_resolve(
    MockGlobalize& globalize,
    frame_plus_local_globalize& globalize_stub,
    MockResolve& resolve,
    global_index_var_resolve& resolve_stub) {
    ON_CALL(globalize, globalize(testing::_, testing::_))
        .WillByDefault(testing::Invoke(&globalize_stub, &frame_plus_local_globalize::globalize));
    ON_CALL(resolve, resolve(testing::_))
        .WillByDefault(testing::Invoke(&resolve_stub, &global_index_var_resolve::resolve));
}

} // namespace

struct PudUnifierTest : public ::testing::Test {
    expr_fixture terms;
    frame_plus_local_globalize globalize_stub;
    global_index_var_resolve resolve_stub{terms};
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

    void SetUp() override {
        stub_globalize_and_resolve(globalize, globalize_stub, resolve, resolve_stub);
    }
};

TEST_F(PudUnifierTest, StructurallyEqualGroundTermsNeedNoBindOps) {
    const expr* other_atom = terms.functor(10u, {});
    ::testing::StrictMock<MockEquate> strict_equate;
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    using strict_unifier_t = unifier<
        ::testing::NiceMock<MockGlobalize>,
        ::testing::NiceMock<MockNormalize>,
        ::testing::NiceMock<MockResolve>,
        ::testing::StrictMock<MockEquate>,
        ::testing::StrictMock<MockInstantiate>>;
    strict_unifier_t strict_unifier{globalize, normalize, resolve, strict_equate, strict_instantiate};
    EXPECT_TRUE(strict_unifier.unify({atom, 0u}, {other_atom, 0u}));
}

TEST_F(PudUnifierTest, SameResolvedVarIndexSkipsEquate) {
    const expr* shared_rep = terms.var(3u);
    ON_CALL(resolve, resolve(testing::_)).WillByDefault(testing::Return(shared_rep));
    ::testing::StrictMock<MockEquate> strict_equate;
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    using strict_unifier_t = unifier<
        ::testing::NiceMock<MockGlobalize>,
        ::testing::NiceMock<MockNormalize>,
        ::testing::NiceMock<MockResolve>,
        ::testing::StrictMock<MockEquate>,
        ::testing::StrictMock<MockInstantiate>>;
    strict_unifier_t strict_unifier{globalize, normalize, resolve, strict_equate, strict_instantiate};
    EXPECT_TRUE(strict_unifier.unify({var0, 0u}, {var1, 0u}));
}

TEST_F(PudUnifierTest, UnifyTwoVarsEquatesIndices) {
    EXPECT_CALL(equate, equate(0u, 1u));
    EXPECT_TRUE(sut.unify({var0, 0u}, {var1, 0u}));
}

TEST_F(PudUnifierTest, UnifyVarWithFunctorNormalizesThenInstantiates) {
    ON_CALL(normalize, normalize(testing::_)).WillByDefault(testing::Return(atom));
    EXPECT_CALL(instantiate, instantiate(2u, atom));
    EXPECT_TRUE(sut.unify({var2, 0u}, {atom, 0u}));
}

TEST_F(PudUnifierTest, OccursCheckRejectsBinding) {
    const expr* cyclic = terms.functor(1u, {var0});
    ON_CALL(normalize, normalize(testing::_)).WillByDefault(testing::Return(cyclic));
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    unifier_no_instantiate_t strict_sut{globalize, normalize, resolve, equate, strict_instantiate};
    EXPECT_FALSE(strict_sut.unify({var0, 0u}, {cyclic, 0u}));
}

TEST_F(PudUnifierTest, MismatchedFunctorIdsFail) {
    EXPECT_FALSE(sut.unify({f0, 0u}, {g0, 0u}));
}

TEST_F(PudUnifierTest, MismatchedArityFails) {
    EXPECT_FALSE(sut.unify({f0, 0u}, {f2, 0u}));
}

struct PudUnifierNestedTest : public ::testing::Test {
    expr_fixture terms;
    frame_plus_local_globalize globalize_stub;
    global_index_var_resolve resolve_stub{terms};
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

    void SetUp() override {
        stub_globalize_and_resolve(globalize, globalize_stub, resolve, resolve_stub);
    }
};

TEST_F(PudUnifierNestedTest, DeepMismatchFails) {
    EXPECT_FALSE(sut.unify({outer_l, 0u}, {outer_r, 0u}));
}

TEST_F(PudUnifierTest, VarLeftFunctorRightMirrorsFunctorLeftVarRight) {
    ::testing::NiceMock<MockGlobalize> g_left;
    ::testing::NiceMock<MockGlobalize> g_right;
    ::testing::NiceMock<MockNormalize> n_left;
    ::testing::NiceMock<MockNormalize> n_right;
    frame_plus_local_globalize g_left_stub;
    frame_plus_local_globalize g_right_stub;
    global_index_var_resolve r_left_stub{terms};
    global_index_var_resolve r_right_stub{terms};
    ::testing::NiceMock<MockResolve> r_left;
    ::testing::NiceMock<MockResolve> r_right;
    ::testing::NiceMock<MockEquate> e_left;
    ::testing::NiceMock<MockEquate> e_right;
    ::testing::NiceMock<MockInstantiate> i_left;
    ::testing::NiceMock<MockInstantiate> i_right;
    test_unifier_t left_unifier{g_left, n_left, r_left, e_left, i_left};
    test_unifier_t right_unifier{g_right, n_right, r_right, e_right, i_right};
    stub_globalize_and_resolve(g_left, g_left_stub, r_left, r_left_stub);
    stub_globalize_and_resolve(g_right, g_right_stub, r_right, r_right_stub);
    ON_CALL(n_left, normalize(testing::_)).WillByDefault(testing::Return(atom));
    ON_CALL(n_right, normalize(testing::_)).WillByDefault(testing::Return(atom));
    EXPECT_CALL(i_left, instantiate(2u, atom));
    EXPECT_CALL(i_right, instantiate(2u, atom));

    EXPECT_TRUE(left_unifier.unify({var2, 0u}, {atom, 0u}));
    EXPECT_TRUE(right_unifier.unify({atom, 0u}, {var2, 0u}));
}

TEST_F(PudUnifierTest, VarVarBothOrdersEquateSameIndices) {
    EXPECT_CALL(equate, equate(0u, 1u));
    EXPECT_TRUE(sut.unify({var0, 0u}, {var1, 0u}));

    ::testing::NiceMock<MockEquate> equate_ba;
    test_unifier_t reverse{globalize, normalize, resolve, equate_ba, instantiate};
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
    const expr* other_atom = terms.functor(10u, {});
    ::testing::StrictMock<MockGlobalize> strict_globalize;
    ::testing::StrictMock<MockNormalize> strict_normalize;
    ::testing::StrictMock<MockResolve> strict_resolve;
    ::testing::StrictMock<MockEquate> strict_equate;
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    strict_unifier_t strict{
        strict_globalize, strict_normalize, strict_resolve, strict_equate, strict_instantiate};
    EXPECT_TRUE(strict.unify({atom, 0u}, {other_atom, 0u}));
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
    ON_CALL(resolve, resolve(testing::_)).WillByDefault(testing::Return(bound));
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    unifier_no_instantiate_t strict_sut{globalize, normalize, resolve, equate, strict_instantiate};
    EXPECT_TRUE(strict_sut.unify({var0, 0u}, {bound, 0u}));
}

TEST_F(PudUnifierTest, TwoVarsBoundToSamePointerShortCircuits) {
    const expr* bound = terms.functor(10u, {});
    ON_CALL(resolve, resolve(testing::_)).WillByDefault(testing::Return(bound));
    ::testing::StrictMock<MockEquate> strict_equate;
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    unifier_no_bind_ops_t strict_sut{globalize, normalize, resolve, strict_equate, strict_instantiate};
    EXPECT_TRUE(strict_sut.unify({var0, 0u}, {var1, 0u}));
}

TEST_F(PudUnifierTest, OccursDeepCycleRejectsBinding) {
    const expr* h = terms.functor(3u, {terms.var(0u)});
    const expr* g = terms.functor(2u, {h});
    const expr* f = terms.functor(1u, {g});
    ON_CALL(normalize, normalize(testing::_)).WillByDefault(testing::Return(f));
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    unifier_no_instantiate_t strict_sut{globalize, normalize, resolve, equate, strict_instantiate};
    EXPECT_FALSE(strict_sut.unify({var0, 0u}, {f, 0u}));
}

TEST_F(PudUnifierTest, FailedOccursPerformsNoInstantiate) {
    const expr* cyclic = terms.functor(1u, {var0});
    ON_CALL(normalize, normalize(testing::_)).WillByDefault(testing::Return(cyclic));
    ::testing::StrictMock<MockInstantiate> strict_instantiate;
    unifier_no_instantiate_t strict_sut{globalize, normalize, resolve, equate, strict_instantiate};
    EXPECT_FALSE(strict_sut.unify({var0, 0u}, {cyclic, 0u}));
}

TEST_F(PudUnifierTest, AfterFailedOccursLegalBindingStillWorks) {
    const expr* cyclic = terms.functor(1u, {var0});
    ON_CALL(normalize, normalize(testing::_)).WillByDefault(testing::Return(cyclic));
    EXPECT_FALSE(sut.unify({var0, 0u}, {cyclic, 0u}));

    ON_CALL(normalize, normalize(testing::_)).WillByDefault(testing::Return(atom));
    EXPECT_CALL(instantiate, instantiate(0u, atom));
    EXPECT_TRUE(sut.unify({var0, 0u}, {atom, 0u}));
}
