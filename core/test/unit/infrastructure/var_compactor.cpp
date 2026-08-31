// var_compactor: renumbers globalized variable indices to contiguous 0..K-1.
// SUT is var_compactor; IMakeFunctor and IMakeVar are GMock mocks.

#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/var_compactor.hpp"
#include "functor_fixture.hpp"

using ::testing::Return;

struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor, (uint32_t, std::vector<const expr*>));
};

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t));
};

using sut_t = var_compactor<MockMakeFunctor, MockMakeVar>;

struct VarCompactorTest : public ::testing::Test {
    test_functors                          functors;
    MockMakeFunctor                        mock_make_functor;
    MockMakeVar                            mock_make_var;
    sut_t                                  sut{mock_make_functor, mock_make_var};
    std::unordered_map<uint32_t, uint32_t> var_map;
    uint32_t                               next_idx = 0;
};

// ---------------------------------------------------------------------------
// Variable input — delegates the compacted index to make_var
// ---------------------------------------------------------------------------

TEST_F(VarCompactorTest, VarAtHighIndexDelegatesZeroToMakeVar) {
    expr raw{expr::var{42}};
    expr result{expr::var{0}};

    EXPECT_CALL(mock_make_var, make_var(0u)).WillOnce(Return(&result));

    const expr* out = sut.compact_vars(&raw, var_map, next_idx);

    EXPECT_EQ(out, &result);
    EXPECT_EQ(next_idx, 1u);
}

TEST_F(VarCompactorTest, VarAtIndexZeroDelegatesZeroToMakeVar) {
    expr raw{expr::var{0}};
    expr result{expr::var{0}};

    EXPECT_CALL(mock_make_var, make_var(0u)).WillOnce(Return(&result));

    sut.compact_vars(&raw, var_map, next_idx);

    EXPECT_EQ(next_idx, 1u);
}

// ---------------------------------------------------------------------------
// Nullary functor — delegates empty args to make_functor; make_var not called
// ---------------------------------------------------------------------------

TEST_F(VarCompactorTest, NullaryFunctorDelegatesEmptyArgsToMakeFunctor) {
    expr raw{expr::functor{functors.id("f"), {}}};
    expr result{expr::functor{functors.id("f"), {}}};

    EXPECT_CALL(mock_make_functor,
                make_functor(functors.id("f"), std::vector<const expr*>{}))
        .WillOnce(Return(&result));

    const expr* out = sut.compact_vars(&raw, var_map, next_idx);

    EXPECT_EQ(out, &result);
    EXPECT_EQ(next_idx, 0u);
}

// ---------------------------------------------------------------------------
// Functor with one var — calls make_var then passes result to make_functor
// ---------------------------------------------------------------------------

TEST_F(VarCompactorTest, FunctorWithVarPassesCompactedChildToMakeFunctor) {
    expr v_raw{expr::var{5}};
    expr raw{expr::functor{functors.id("f"), {&v_raw}}};
    expr v_compact{expr::var{0}};
    expr f_result{expr::functor{functors.id("f"), {&v_compact}}};

    EXPECT_CALL(mock_make_var, make_var(0u)).WillOnce(Return(&v_compact));
    EXPECT_CALL(mock_make_functor,
                make_functor(functors.id("f"), std::vector<const expr*>{&v_compact}))
        .WillOnce(Return(&f_result));

    const expr* out = sut.compact_vars(&raw, var_map, next_idx);

    EXPECT_EQ(out, &f_result);
    EXPECT_EQ(next_idx, 1u);
}

// ---------------------------------------------------------------------------
// Same var twice — make_var called twice with the same compacted index
// ---------------------------------------------------------------------------

TEST_F(VarCompactorTest, SameVarTwiceDelegatesSameIndexBothTimes) {
    expr v_raw{expr::var{7}};
    expr raw{expr::functor{functors.id("f"), {&v_raw, &v_raw}}};
    expr r0{expr::var{0}};
    expr r1{expr::var{0}};
    expr f_result{expr::functor{functors.id("f"), {&r0, &r1}}};

    EXPECT_CALL(mock_make_var, make_var(0u))
        .WillOnce(Return(&r0))
        .WillOnce(Return(&r1));
    EXPECT_CALL(mock_make_functor,
                make_functor(functors.id("f"), std::vector<const expr*>{&r0, &r1}))
        .WillOnce(Return(&f_result));

    sut.compact_vars(&raw, var_map, next_idx);

    EXPECT_EQ(next_idx, 1u);
}

// ---------------------------------------------------------------------------
// Two distinct vars — delegated in first-appearance order (0, then 1)
// ---------------------------------------------------------------------------

TEST_F(VarCompactorTest, TwoDistinctVarsDelegateAscendingIndices) {
    expr v5{expr::var{5}};
    expr v10{expr::var{10}};
    expr raw{expr::functor{functors.id("f"), {&v5, &v10}}};
    expr r0{expr::var{0}};
    expr r1{expr::var{1}};
    expr f_result{expr::functor{functors.id("f"), {&r0, &r1}}};

    EXPECT_CALL(mock_make_var, make_var(0u)).WillOnce(Return(&r0));
    EXPECT_CALL(mock_make_var, make_var(1u)).WillOnce(Return(&r1));
    EXPECT_CALL(mock_make_functor,
                make_functor(functors.id("f"), std::vector<const expr*>{&r0, &r1}))
        .WillOnce(Return(&f_result));

    sut.compact_vars(&raw, var_map, next_idx);

    EXPECT_EQ(next_idx, 2u);
}

TEST_F(VarCompactorTest, FirstAppearanceWinsRegardlessOfRawIndex) {
    // var(10) appears first in args, so it gets compacted index 0; var(5) gets 1
    expr v10{expr::var{10}};
    expr v5{expr::var{5}};
    expr raw{expr::functor{functors.id("f"), {&v10, &v5}}};
    expr r0{expr::var{0}};
    expr r1{expr::var{1}};
    expr f_result{expr::functor{functors.id("f"), {&r0, &r1}}};

    EXPECT_CALL(mock_make_var, make_var(0u)).WillOnce(Return(&r0));
    EXPECT_CALL(mock_make_var, make_var(1u)).WillOnce(Return(&r1));
    EXPECT_CALL(mock_make_functor,
                make_functor(functors.id("f"), std::vector<const expr*>{&r0, &r1}))
        .WillOnce(Return(&f_result));

    sut.compact_vars(&raw, var_map, next_idx);

    EXPECT_EQ(next_idx, 2u);
}

// ---------------------------------------------------------------------------
// Shared var_map across two calls — same raw var produces same compacted index
// ---------------------------------------------------------------------------

TEST_F(VarCompactorTest, SharedVarMapYieldsSameIndexOnSecondCall) {
    expr v_raw{expr::var{5}};
    expr r0{expr::var{0}};
    expr r1{expr::var{0}};

    EXPECT_CALL(mock_make_var, make_var(0u))
        .WillOnce(Return(&r0))
        .WillOnce(Return(&r1));

    sut.compact_vars(&v_raw, var_map, next_idx);
    sut.compact_vars(&v_raw, var_map, next_idx);

    EXPECT_EQ(next_idx, 1u);
}

// ---------------------------------------------------------------------------
// Nested functor — recursion is bottom-up; inner make calls precede outer
// ---------------------------------------------------------------------------

TEST_F(VarCompactorTest, NestedFunctorDelegatesBottomUp) {
    expr v_raw{expr::var{9}};
    expr inner_raw{expr::functor{functors.id("f"), {&v_raw}}};
    expr outer_raw{expr::functor{functors.id("g"), {&inner_raw}}};

    expr v_compact{expr::var{0}};
    expr inner_compact{expr::functor{functors.id("f"), {&v_compact}}};
    expr outer_compact{expr::functor{functors.id("g"), {&inner_compact}}};

    EXPECT_CALL(mock_make_var, make_var(0u)).WillOnce(Return(&v_compact));
    EXPECT_CALL(mock_make_functor,
                make_functor(functors.id("f"), std::vector<const expr*>{&v_compact}))
        .WillOnce(Return(&inner_compact));
    EXPECT_CALL(mock_make_functor,
                make_functor(functors.id("g"), std::vector<const expr*>{&inner_compact}))
        .WillOnce(Return(&outer_compact));

    const expr* out = sut.compact_vars(&outer_raw, var_map, next_idx);

    EXPECT_EQ(out, &outer_compact);
    EXPECT_EQ(next_idx, 1u);
}
