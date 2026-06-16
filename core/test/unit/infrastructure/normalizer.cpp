// normalizer rebuilds expressions in WHNF via i_make_functor after resolving variables
// through i_bind_map. Globalization of whnf results is delegated to i_globalizer.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/normalizer.hpp"
#include "interfaces/i_bind_map.hpp"
#include "interfaces/i_globalizer.hpp"
#include "interfaces/i_make_functor.hpp"
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
    MOCK_METHOD(const expr*, globalize, (framed_expr), (override));
};

struct MockMakeFunctor : public i_make_functor {
    MOCK_METHOD(const expr*, make_functor, (uint32_t, const std::vector<const expr*>&), (override));
};

struct NormalizerUnitTest : public ::testing::Test {
protected:
    test_functors functors;

    void SetUp() override {
        ON_CALL(bm, whnf(_)).WillByDefault([this](framed_expr fe) -> framed_expr {
            if (fe.skeleton == &var0) return {&arg_whnf0, 0};
            if (fe.skeleton == &var1) return {&arg_whnf1, 0};
            if (fe.skeleton == &var2) return {&arg_whnf2, 0};
            return fe;
        });
        ON_CALL(glob, globalize(_)).WillByDefault([](framed_expr fe) -> const expr* {
            return fe.skeleton;
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
        loc.bind_as<i_bind_map>(bm);
        return normalizer{loc};
    }

    locator loc;
    NiceMock<MockBindMap> bm;
    NiceMock<MockGlobalizer> glob;
    NiceMock<MockMakeFunctor> pool;

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr var_repr{expr::var{5}};
    expr f_raw{expr::functor{functors.id("f"), {&var0}}};
    expr g_var0{expr::functor{functors.id("g"), {&var0}}};
    expr f_g_var0{expr::functor{functors.id("f"), {&g_var0}}};

    expr pooled_f{expr::functor{functors.id("f"), {}}};
    expr pooled_g{expr::functor{functors.id("g"), {}}};
    // arg_whnf* are the result of whnf on var0/var1/var2
    expr arg_whnf0{expr::var{90}};
    expr arg_whnf1{expr::var{91}};
    expr arg_whnf2{expr::var{92}};
};

// ---------------------------------------------------------------------------
// Var — globalize is called, if result is a var we return it directly
// ---------------------------------------------------------------------------

TEST_F(NormalizerUnitTest, UnboundVarCallsGlobalizeAndReturns) {
    // whnf returns a var → globalize is called → result returned directly
    ON_CALL(bm, whnf(_)).WillByDefault(Return(framed_expr{&var0, 0}));
    expr globalized_var{expr::var{42}};
    EXPECT_CALL(glob, globalize(framed_expr{&var0, 0})).WillOnce(Return(&globalized_var));
    EXPECT_CALL(pool, make_functor(_, _)).Times(0);

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize(&var0), &globalized_var);
}

TEST_F(NormalizerUnitTest, BoundVarGlobalizesToFunctorThenNormalizesArgs) {
    // var0 is bound: whnf returns f_raw (a functor); globalize turns it into globalized_f.
    // normalize then recurses on globalized_f's arg (var0 again, but now unbound).
    expr globalized_f{expr::functor{functors.id("f"), {&var0}}};
    expr globalized_v{expr::var{0}};

    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0}))
        .WillOnce(Return(framed_expr{&f_raw, 0}))   // first call: var0 bound to f_raw
        .WillRepeatedly(Return(framed_expr{&var0, 0})); // subsequent: var0 unbound
    EXPECT_CALL(glob, globalize(framed_expr{&f_raw, 0})).WillOnce(Return(&globalized_f));
    EXPECT_CALL(glob, globalize(framed_expr{&var0, 0})).WillOnce(Return(&globalized_v));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&globalized_v)))
        .WillOnce(Return(&pooled_f));

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize(&var0), &pooled_f);
}

// ---------------------------------------------------------------------------
// Functors — already global, normalize each arg
// ---------------------------------------------------------------------------

TEST_F(NormalizerUnitTest, FunctorNormalizesAllArgs) {
    expr f2{expr::functor{functors.id("f"), {&var0, &var1}}};

    EXPECT_CALL(glob, globalize(framed_expr{&arg_whnf0, 0})).WillOnce(Return(&arg_whnf0));
    EXPECT_CALL(glob, globalize(framed_expr{&arg_whnf1, 0})).WillOnce(Return(&arg_whnf1));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&arg_whnf0, &arg_whnf1)))
        .WillOnce(Return(&pooled_f));

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize(&f2), &pooled_f);
}

TEST_F(NormalizerUnitTest, NestedFunctorNormalizesInnerArg) {
    EXPECT_CALL(glob, globalize(framed_expr{&arg_whnf0, 0})).WillOnce(Return(&arg_whnf0));
    EXPECT_CALL(pool, make_functor(functors.id("g"), ElementsAre(&arg_whnf0)))
        .WillOnce(Return(&pooled_g));
    EXPECT_CALL(pool, make_functor(functors.id("f"), ElementsAre(&pooled_g)))
        .WillOnce(Return(&pooled_f));

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize(&f_g_var0), &pooled_f);
}
