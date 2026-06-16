// normalizer rebuilds expressions in WHNF via i_make_functor after resolving variables
// through i_bind_map. Variable indices are globalized via i_globalizer + i_make_var.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/normalizer.hpp"
#include "interfaces/i_bind_map.hpp"
#include "interfaces/i_globalizer.hpp"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"
#include "functor_fixture.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;

struct MockBindMap : public i_bind_map {
    MOCK_METHOD(void,        bind, (uint32_t, framed_expr), (override));
    MOCK_METHOD(framed_expr, whnf, (framed_expr),           (override));
};

struct MockGlobalizer : public i_globalizer {
    MOCK_METHOD(uint32_t, globalize, (uint32_t, uint32_t), (override));
};

struct MockMakeVar : public i_make_var {
    MOCK_METHOD(const expr*, make_var, (uint32_t), (override));
};

struct MockMakeFunctor : public i_make_functor {
    MOCK_METHOD(const expr*, make_functor, (uint32_t, const std::vector<const expr*>&), (override));
};

struct NormalizerUnitTest : public ::testing::Test {
protected:
    test_functors functors;

    void SetUp() override {
        ON_CALL(bm, whnf(_)).WillByDefault([](framed_expr fe) { return fe; });
        ON_CALL(glob, globalize(_, _)).WillByDefault([](uint32_t off, uint32_t idx) {
            return off + idx;
        });
        ON_CALL(mkvar, make_var(_)).WillByDefault([this](uint32_t idx) -> const expr* {
            return idx == 0 ? &var0 : idx == 1 ? &var1 : &var2;
        });
        ON_CALL(pool, make_functor(_, _)).WillByDefault([this](uint32_t id,
            const std::vector<const expr*>&) -> const expr* {
            if (id == functors.id("g")) return &pooled_g;
            if (id == functors.id("f")) return &pooled_f;
            return nullptr;
        });
    }

    normalizer make_normalizer() {
        loc.bind_as<i_globalizer>(glob);
        loc.bind_as<i_make_functor>(pool);
        loc.bind_as<i_make_var>(mkvar);
        loc.bind_as<i_bind_map>(bm);
        return normalizer{loc};
    }

    locator loc;
    NiceMock<MockBindMap> bm;
    NiceMock<MockGlobalizer> glob;
    NiceMock<MockMakeVar> mkvar;
    NiceMock<MockMakeFunctor> pool;

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr f_raw{expr::functor{functors.id("f"), {&var0}}};
    expr g_var0{expr::functor{functors.id("g"), {&var0}}};
    expr f_g_var0{expr::functor{functors.id("f"), {&g_var0}}};

    expr pooled_f{expr::functor{functors.id("f"), {}}};
    expr pooled_g{expr::functor{functors.id("g"), {}}};
};

// ---------------------------------------------------------------------------
// Var — whnf resolves, globalize computes index, make_var produces expr
// ---------------------------------------------------------------------------

TEST_F(NormalizerUnitTest, UnboundVarGlobalizesAndCallsMakeVar) {
    expr result_var{expr::var{42}};
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillOnce(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(glob, globalize(0u, 0u)).WillOnce(Return(42u));
    EXPECT_CALL(mkvar, make_var(42u)).WillOnce(Return(&result_var));
    EXPECT_CALL(pool, make_functor(_, _)).Times(0);

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize({&var0, 0}), &result_var);
}

TEST_F(NormalizerUnitTest, VarWithNonZeroFrameOffsetGlobalizesCorrectly) {
    expr result_var{expr::var{12}};
    EXPECT_CALL(bm, whnf(framed_expr{&var2, 10})).WillOnce(Return(framed_expr{&var2, 10}));
    EXPECT_CALL(glob, globalize(10u, 2u)).WillOnce(Return(12u));
    EXPECT_CALL(mkvar, make_var(12u)).WillOnce(Return(&result_var));

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize({&var2, 10}), &result_var);
}

TEST_F(NormalizerUnitTest, BoundVarResolvesToFunctorThenNormalizesArgs) {
    // var0 resolves via whnf to f_raw (a functor containing var0 as arg).
    // The functor's args are then recursively normalized.
    expr globalized_var{expr::var{0}};

    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0}))
        .WillOnce(Return(framed_expr{&f_raw, 0}))
        .WillOnce(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(glob, globalize(0u, 0u)).WillOnce(Return(0u));
    EXPECT_CALL(mkvar, make_var(0u)).WillOnce(Return(&globalized_var));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&globalized_var)))
        .WillOnce(Return(&pooled_f));

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize({&var0, 0}), &pooled_f);
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
    EXPECT_CALL(glob, globalize(0u, 0u)).WillOnce(Return(0u));
    EXPECT_CALL(glob, globalize(0u, 1u)).WillOnce(Return(1u));
    EXPECT_CALL(mkvar, make_var(0u)).WillOnce(Return(&r0));
    EXPECT_CALL(mkvar, make_var(1u)).WillOnce(Return(&r1));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&r0, &r1)))
        .WillOnce(Return(&pooled_f));

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize({&f2, 0}), &pooled_f);
}

TEST_F(NormalizerUnitTest, NestedFunctorNormalizesInnerArg) {
    expr inner_var{expr::var{0}};

    EXPECT_CALL(bm, whnf(framed_expr{&f_g_var0, 0})).WillOnce(Return(framed_expr{&f_g_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&g_var0, 0})).WillOnce(Return(framed_expr{&g_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillOnce(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(glob, globalize(0u, 0u)).WillOnce(Return(0u));
    EXPECT_CALL(mkvar, make_var(0u)).WillOnce(Return(&inner_var));
    EXPECT_CALL(pool, make_functor(functors.id("g"), ElementsAre(&inner_var)))
        .WillOnce(Return(&pooled_g));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&pooled_g)))
        .WillOnce(Return(&pooled_f));

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize({&f_g_var0, 0}), &pooled_f);
}
