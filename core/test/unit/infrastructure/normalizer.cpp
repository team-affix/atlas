// normalizer rebuilds expressions in WHNF after resolving variables
// through bind_map. Variable indices are globalized via globalizer + expr_pool.
// Vars below cutoff skip WHNF and keep their index; leftover vars at/above
// cutoff are remapped via a translation map to cutoff + size() at insert.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_map>
#include "infrastructure/normalizer.hpp"
#include "functor_fixture.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;

struct MockBindMap {
    MOCK_METHOD(void,        bind, (uint32_t, framed_expr));
    MOCK_METHOD(framed_expr, whnf, (framed_expr));
};

struct MockGlobalizer {
    MOCK_METHOD(uint32_t, globalize, (uint32_t, uint32_t), (const));
};

struct MockExprPool {
    MOCK_METHOD(const expr*, make_var,     (uint32_t));
    MOCK_METHOD(const expr*, make_functor, (uint32_t, const std::vector<const expr*>&));
};

using test_normalizer_t = normalizer<MockGlobalizer, MockExprPool, MockExprPool, MockBindMap>;

struct NormalizerUnitTest : public ::testing::Test {
protected:
    test_functors functors;

    void SetUp() override {
        ON_CALL(bm, whnf(_)).WillByDefault([](framed_expr fe) { return fe; });
        ON_CALL(glob, globalize(_, _)).WillByDefault([](uint32_t off, uint32_t idx) {
            return off + idx;
        });
        ON_CALL(pool, make_var(_)).WillByDefault([this](uint32_t idx) -> const expr* {
            switch (idx) {
                case 0: return &var0;
                case 1: return &var1;
                case 2: return &var2;
                case 3: return &var3;
                case 4: return &var4;
                case 5: return &var5;
                case 10: return &var10;
                default: return &var0;
            }
        });
        ON_CALL(pool, make_functor(_, _)).WillByDefault([this](uint32_t id,
            const std::vector<const expr*>&) -> const expr* {
            if (id == functors.id("g")) return &pooled_g;
            if (id == functors.id("f")) return &pooled_f;
            if (id == functors.id("h")) return &pooled_h;
            return nullptr;
        });
    }

    const expr* normalize0(framed_expr fe) {
        std::unordered_map<uint32_t, uint32_t> translation;
        return norm.normalize(fe, 0, translation);
    }

    NiceMock<MockBindMap> bm;
    NiceMock<MockGlobalizer> glob;
    NiceMock<MockExprPool> pool;

    test_normalizer_t norm{glob, pool, pool, bm};

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr var3{expr::var{3}};
    expr var4{expr::var{4}};
    expr var5{expr::var{5}};
    expr var10{expr::var{10}};
    expr f_raw{expr::functor{functors.id("f"), {&var0}}};
    expr g_var0{expr::functor{functors.id("g"), {&var0}}};
    expr f_g_var0{expr::functor{functors.id("f"), {&g_var0}}};

    expr pooled_f{expr::functor{functors.id("f"), {}}};
    expr pooled_g{expr::functor{functors.id("g"), {}}};
    expr pooled_h{expr::functor{functors.id("h"), {}}};
};

// ---------------------------------------------------------------------------
// Var — whnf resolves, leftover indices are remapped from 0
// ---------------------------------------------------------------------------

TEST_F(NormalizerUnitTest, UnboundVarGlobalizesAndCallsMakeVar) {
    expr result_var{expr::var{0}};
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillOnce(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(glob, globalize(0u, 0u)).Times(2).WillRepeatedly(Return(42u));
    EXPECT_CALL(pool, make_var(0u)).WillOnce(Return(&result_var));
    EXPECT_CALL(pool, make_functor(_, _)).Times(0);

    EXPECT_EQ(normalize0({&var0, 0}), &result_var);
}

TEST_F(NormalizerUnitTest, VarWithNonZeroFrameOffsetGlobalizesCorrectly) {
    expr result_var{expr::var{0}};
    EXPECT_CALL(bm, whnf(framed_expr{&var2, 10})).WillOnce(Return(framed_expr{&var2, 10}));
    EXPECT_CALL(glob, globalize(10u, 2u)).Times(2).WillRepeatedly(Return(12u));
    EXPECT_CALL(pool, make_var(0u)).WillOnce(Return(&result_var));

    EXPECT_EQ(normalize0({&var2, 10}), &result_var);
}

TEST_F(NormalizerUnitTest, BoundVarResolvesToFunctorThenNormalizesArgs) {
    // var0 resolves via whnf to f_raw (a functor containing var0 as arg).
    // The functor's args are then recursively normalized.
    expr globalized_var{expr::var{0}};

    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0}))
        .WillOnce(Return(framed_expr{&f_raw, 0}))
        .WillOnce(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(glob, globalize(0u, 0u)).Times(3).WillRepeatedly(Return(0u));
    EXPECT_CALL(pool, make_var(0u)).WillOnce(Return(&globalized_var));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&globalized_var)))
        .WillOnce(Return(&pooled_f));

    EXPECT_EQ(normalize0({&var0, 0}), &pooled_f);
}

// ---------------------------------------------------------------------------
// Functors — whnf returns self, normalize each arg
// ---------------------------------------------------------------------------

TEST_F(NormalizerUnitTest, FunctorNormalizesAllArgs) {
    expr f2{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr r0{expr::var{0}};
    expr r1{expr::var{1}};

    EXPECT_CALL(bm, whnf(framed_expr{&f2, 0})).WillOnce(Return(framed_expr{&f2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillOnce(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillOnce(Return(framed_expr{&var1, 0}));
    EXPECT_CALL(glob, globalize(0u, 0u)).Times(2).WillRepeatedly(Return(0u));
    EXPECT_CALL(glob, globalize(0u, 1u)).Times(2).WillRepeatedly(Return(1u));
    EXPECT_CALL(pool, make_var(0u)).WillOnce(Return(&r0));
    EXPECT_CALL(pool, make_var(1u)).WillOnce(Return(&r1));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&r0, &r1)))
        .WillOnce(Return(&pooled_f));

    EXPECT_EQ(normalize0({&f2, 0}), &pooled_f);
}

TEST_F(NormalizerUnitTest, NestedFunctorNormalizesInnerArg) {
    expr inner_var{expr::var{0}};

    EXPECT_CALL(bm, whnf(framed_expr{&f_g_var0, 0})).WillOnce(Return(framed_expr{&f_g_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&g_var0, 0})).WillOnce(Return(framed_expr{&g_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillOnce(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(glob, globalize(0u, 0u)).Times(2).WillRepeatedly(Return(0u));
    EXPECT_CALL(pool, make_var(0u)).WillOnce(Return(&inner_var));
    EXPECT_CALL(pool, make_functor(functors.id("g"), ElementsAre(&inner_var)))
        .WillOnce(Return(&pooled_g));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&pooled_g)))
        .WillOnce(Return(&pooled_f));

    EXPECT_EQ(normalize0({&f_g_var0, 0}), &pooled_f);
}

// ---------------------------------------------------------------------------
// Cutoff — freeze below cutoff, remap leftover high vars
// ---------------------------------------------------------------------------

TEST_F(NormalizerUnitTest, CutoffKeepsLowVarsAndRemapsHighInEncounterOrder) {
    expr g01{expr::functor{functors.id("g"), {&var0, &var1}}};
    expr h45{expr::functor{functors.id("h"), {&var4, &var5}}};
    expr fgh{expr::functor{functors.id("f"), {&g01, &h45}}};
    expr r0{expr::var{0}};
    expr r1{expr::var{1}};
    expr r2{expr::var{2}};
    expr r3{expr::var{3}};

    EXPECT_CALL(pool, make_var(0u)).WillOnce(Return(&r0));
    EXPECT_CALL(pool, make_var(1u)).WillOnce(Return(&r1));
    EXPECT_CALL(pool, make_var(2u)).WillOnce(Return(&r2));
    EXPECT_CALL(pool, make_var(3u)).WillOnce(Return(&r3));
    EXPECT_CALL(pool, make_functor(functors.id("g"), ElementsAre(&r0, &r1)))
        .WillOnce(Return(&pooled_g));
    EXPECT_CALL(pool, make_functor(functors.id("h"), ElementsAre(&r2, &r3)))
        .WillOnce(Return(&pooled_h));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&pooled_g, &pooled_h)))
        .WillOnce(Return(&pooled_f));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&fgh, 0}, 2, translation), &pooled_f);
}

TEST_F(NormalizerUnitTest, SameLeftoverVarReusesTranslation) {
    expr f44{expr::functor{functors.id("f"), {&var4, &var4}}};
    expr r2{expr::var{2}};

    EXPECT_CALL(pool, make_var(2u)).Times(2).WillRepeatedly(Return(&r2));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&r2, &r2)))
        .WillOnce(Return(&pooled_f));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&f44, 0}, 2, translation), &pooled_f);
    EXPECT_EQ(translation.size(), 1u);
    EXPECT_EQ(translation.at(4u), 2u);
}

TEST_F(NormalizerUnitTest, FrozenVarBelowCutoffDoesNotCallWhnf) {
    EXPECT_CALL(bm, whnf(_)).Times(0);
    EXPECT_CALL(pool, make_var(0u)).WillOnce(Return(&var0));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&var0, 0}, 2, translation), &var0);
    EXPECT_TRUE(translation.empty());
}

TEST_F(NormalizerUnitTest, HighVarThatWhnfsToFunctorNormalizesArgs) {
    EXPECT_CALL(bm, whnf(framed_expr{&var4, 0}))
        .WillOnce(Return(framed_expr{&f_raw, 0}));
    EXPECT_CALL(pool, make_var(0u)).WillOnce(Return(&var0));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&var0)))
        .WillOnce(Return(&pooled_f));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&var4, 0}, 2, translation), &pooled_f);
}

TEST_F(NormalizerUnitTest, HighVarThatWhnfsToFrozenVarIsEmittedUnremapped) {
    EXPECT_CALL(bm, whnf(framed_expr{&var4, 0}))
        .WillOnce(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(pool, make_var(0u)).WillOnce(Return(&var0));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&var4, 0}, 2, translation), &var0);
    EXPECT_TRUE(translation.empty());
}

TEST_F(NormalizerUnitTest, SecondNewVarIsAssignedCutoffPlusOne) {
    expr f45{expr::functor{functors.id("f"), {&var4, &var5}}};
    expr r2{expr::var{2}};
    expr r3{expr::var{3}};

    EXPECT_CALL(pool, make_var(2u)).WillOnce(Return(&r2));
    EXPECT_CALL(pool, make_var(3u)).WillOnce(Return(&r3));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&r2, &r3)))
        .WillOnce(Return(&pooled_f));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&f45, 0}, 2, translation), &pooled_f);
    EXPECT_EQ(translation.size(), 2u);
    EXPECT_EQ(translation.at(4u), 2u);
    EXPECT_EQ(translation.at(5u), 3u);
}

TEST_F(NormalizerUnitTest, SharedTranslationMapRenamesConsistentlyAcrossCalls) {
    expr r2{expr::var{2}};
    expr r3{expr::var{3}};
    EXPECT_CALL(pool, make_var(2u)).Times(2).WillRepeatedly(Return(&r2));
    EXPECT_CALL(pool, make_var(3u)).WillOnce(Return(&r3));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&var4, 0}, 2, translation), &r2);
    EXPECT_EQ(norm.normalize({&var5, 0}, 2, translation), &r3);
    EXPECT_EQ(norm.normalize({&var4, 0}, 2, translation), &r2);
    EXPECT_EQ(translation.size(), 2u);
}

TEST_F(NormalizerUnitTest, VarAtCutoffIsNotFrozenAndIsRemapped) {
    expr r2{expr::var{2}};
    EXPECT_CALL(bm, whnf(framed_expr{&var2, 0})).WillOnce(Return(framed_expr{&var2, 0}));
    EXPECT_CALL(pool, make_var(2u)).WillOnce(Return(&r2));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&var2, 0}, 2, translation), &r2);
    EXPECT_EQ(translation.at(2u), 2u);
}

TEST_F(NormalizerUnitTest, FrozenCheckUsesGlobalizedIndexNotRawIndex) {
    expr r5{expr::var{5}};
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 10})).WillOnce(Return(framed_expr{&var0, 10}));
    EXPECT_CALL(pool, make_var(5u)).WillOnce(Return(&r5));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&var0, 10}, 5, translation), &r5);
    EXPECT_EQ(translation.at(10u), 5u);
}

TEST_F(NormalizerUnitTest, FrozenVarAtNonZeroFrameSkipsWhnfAndKeepsGlobalizedKey) {
    EXPECT_CALL(bm, whnf(_)).Times(0);
    EXPECT_CALL(pool, make_var(10u)).WillOnce(Return(&var10));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&var0, 10}, 15, translation), &var10);
    EXPECT_TRUE(translation.empty());
}

TEST_F(NormalizerUnitTest, PreseededTranslationIsUsedForMatchingKey) {
    expr r7{expr::var{7}};
    EXPECT_CALL(pool, make_var(7u)).WillOnce(Return(&r7));

    std::unordered_map<uint32_t, uint32_t> translation;
    translation.emplace(4u, 7u);
    EXPECT_EQ(norm.normalize({&var4, 0}, 2, translation), &r7);
    EXPECT_EQ(translation.size(), 1u);
}

TEST_F(NormalizerUnitTest, PreseededTranslationAdvancesAssignmentForNewKeys) {
    expr r3{expr::var{3}};
    EXPECT_CALL(pool, make_var(3u)).WillOnce(Return(&r3));

    std::unordered_map<uint32_t, uint32_t> translation;
    translation.emplace(99u, 2u);
    EXPECT_EQ(norm.normalize({&var4, 0}, 2, translation), &r3);
    EXPECT_EQ(translation.at(4u), 3u);
    EXPECT_EQ(translation.at(99u), 2u);
}

TEST_F(NormalizerUnitTest, FrozenVarIgnoresPreseededTranslationEntry) {
    EXPECT_CALL(bm, whnf(_)).Times(0);
    EXPECT_CALL(pool, make_var(0u)).WillOnce(Return(&var0));

    std::unordered_map<uint32_t, uint32_t> translation;
    translation.emplace(0u, 99u);
    EXPECT_EQ(norm.normalize({&var0, 0}, 2, translation), &var0);
    EXPECT_EQ(translation.at(0u), 99u);
}

TEST_F(NormalizerUnitTest, HighVarThatWhnfsToAnotherHighVarIsRemappedFromResultKey) {
    expr r2{expr::var{2}};
    EXPECT_CALL(bm, whnf(framed_expr{&var5, 0})).WillOnce(Return(framed_expr{&var4, 0}));
    EXPECT_CALL(pool, make_var(2u)).WillOnce(Return(&r2));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&var5, 0}, 2, translation), &r2);
    EXPECT_EQ(translation.size(), 1u);
    EXPECT_EQ(translation.at(4u), 2u);
    EXPECT_FALSE(translation.contains(5u));
}

TEST_F(NormalizerUnitTest, HighVarThatWhnfsToAlreadyTranslatedHighVarReusesEntry) {
    expr r2{expr::var{2}};
    EXPECT_CALL(pool, make_var(2u)).Times(2).WillRepeatedly(Return(&r2));
    EXPECT_CALL(bm, whnf(framed_expr{&var4, 0})).WillOnce(Return(framed_expr{&var4, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var5, 0})).WillOnce(Return(framed_expr{&var4, 0}));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&var4, 0}, 2, translation), &r2);
    EXPECT_EQ(norm.normalize({&var5, 0}, 2, translation), &r2);
    EXPECT_EQ(translation.size(), 1u);
    EXPECT_EQ(translation.at(4u), 2u);
}

TEST_F(NormalizerUnitTest, MixedFrozenAndLeftoverArgsPreserveFrozenAndCompactLeftovers) {
    expr mixed{expr::functor{functors.id("f"), {&var0, &var4, &var1, &var5}}};
    expr r0{expr::var{0}};
    expr r1{expr::var{1}};
    expr r2{expr::var{2}};
    expr r3{expr::var{3}};

    EXPECT_CALL(pool, make_var(0u)).WillOnce(Return(&r0));
    EXPECT_CALL(pool, make_var(2u)).WillOnce(Return(&r2));
    EXPECT_CALL(pool, make_var(1u)).WillOnce(Return(&r1));
    EXPECT_CALL(pool, make_var(3u)).WillOnce(Return(&r3));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&r0, &r2, &r1, &r3)))
        .WillOnce(Return(&pooled_f));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&mixed, 0}, 2, translation), &pooled_f);
}

TEST_F(NormalizerUnitTest, LeftoverEncounterOrderFollowsWalkNotNumericOrder) {
    expr h45{expr::functor{functors.id("h"), {&var4, &var5}}};
    expr g01{expr::functor{functors.id("g"), {&var0, &var1}}};
    expr fhg{expr::functor{functors.id("f"), {&h45, &g01}}};
    expr r0{expr::var{0}};
    expr r1{expr::var{1}};
    expr r2{expr::var{2}};
    expr r3{expr::var{3}};

    EXPECT_CALL(pool, make_var(2u)).WillOnce(Return(&r2));
    EXPECT_CALL(pool, make_var(3u)).WillOnce(Return(&r3));
    EXPECT_CALL(pool, make_var(0u)).WillOnce(Return(&r0));
    EXPECT_CALL(pool, make_var(1u)).WillOnce(Return(&r1));
    EXPECT_CALL(pool, make_functor(functors.id("h"), ElementsAre(&r2, &r3)))
        .WillOnce(Return(&pooled_h));
    EXPECT_CALL(pool, make_functor(functors.id("g"), ElementsAre(&r0, &r1)))
        .WillOnce(Return(&pooled_g));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&pooled_h, &pooled_g)))
        .WillOnce(Return(&pooled_f));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&fhg, 0}, 2, translation), &pooled_f);
    EXPECT_EQ(translation.at(4u), 2u);
    EXPECT_EQ(translation.at(5u), 3u);
}

TEST_F(NormalizerUnitTest, SameRawIndexAtDifferentFramesGetsDistinctTranslations) {
    expr r2{expr::var{2}};
    expr r3{expr::var{3}};
    EXPECT_CALL(pool, make_var(2u)).WillOnce(Return(&r2));
    EXPECT_CALL(pool, make_var(3u)).WillOnce(Return(&r3));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&var4, 0}, 2, translation), &r2);
    EXPECT_EQ(norm.normalize({&var4, 10}, 2, translation), &r3);
    EXPECT_EQ(translation.at(4u), 2u);
    EXPECT_EQ(translation.at(14u), 3u);
}

TEST_F(NormalizerUnitTest, IndependentMapsDoNotShareTranslations) {
    expr r2{expr::var{2}};
    EXPECT_CALL(pool, make_var(2u)).Times(2).WillRepeatedly(Return(&r2));

    std::unordered_map<uint32_t, uint32_t> first;
    std::unordered_map<uint32_t, uint32_t> second;
    EXPECT_EQ(norm.normalize({&var4, 0}, 2, first), &r2);
    EXPECT_EQ(norm.normalize({&var5, 0}, 2, second), &r2);
    EXPECT_EQ(first.at(4u), 2u);
    EXPECT_EQ(second.at(5u), 2u);
    EXPECT_FALSE(first.contains(5u));
    EXPECT_FALSE(second.contains(4u));
}

TEST_F(NormalizerUnitTest, FunctorIsWhnfdEvenWhenArgsAreFrozen) {
    EXPECT_CALL(bm, whnf(framed_expr{&f_raw, 0})).WillOnce(Return(framed_expr{&f_raw, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).Times(0);
    EXPECT_CALL(pool, make_var(0u)).WillOnce(Return(&var0));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&var0)))
        .WillOnce(Return(&pooled_f));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&f_raw, 0}, 2, translation), &pooled_f);
}

TEST_F(NormalizerUnitTest, HighVarWhnfToFunctorAtDifferentFrameRemapsArgs) {
    expr r2{expr::var{2}};
    EXPECT_CALL(bm, whnf(framed_expr{&var4, 0}))
        .WillOnce(Return(framed_expr{&f_raw, 7}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 7}))
        .WillOnce(Return(framed_expr{&var0, 7}));
    EXPECT_CALL(pool, make_var(2u)).WillOnce(Return(&r2));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&r2)))
        .WillOnce(Return(&pooled_f));

    std::unordered_map<uint32_t, uint32_t> translation;
    EXPECT_EQ(norm.normalize({&var4, 0}, 2, translation), &pooled_f);
    EXPECT_EQ(translation.at(7u), 2u);
}
