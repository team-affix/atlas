// normalizer rebuilds expressions in WHNF via i_make_functor after resolving variables
// through i_bind_map. Unit tests mock both interfaces and assert pool calls use WHNF args.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/normalizer.hpp"
#include "interfaces/i_bind_map.hpp"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;

struct MockBindMap : public i_bind_map {
    MOCK_METHOD(void,        bind, (uint32_t, const expr*), (override));
    MOCK_METHOD(const expr*, whnf, (const expr*),           (override));
};

struct MockExprPool : public i_make_functor, public i_make_var {
    MOCK_METHOD(const expr*, make, (const std::string&, const std::vector<const expr*>&), (override));
    MOCK_METHOD(const expr*, make, (uint32_t), (override));
};

struct NormalizerUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
        ON_CALL(bm, whnf(_)).WillByDefault([this](const expr* e) -> const expr* {
            if (e == &var0)
                return &arg_whnf0;
            if (e == &var1)
                return &arg_whnf1;
            if (e == &var2)
                return &arg_whnf2;
            return e;
        });
        ON_CALL(pool, make(::testing::An<uint32_t>())).WillByDefault(Return(nullptr));
        ON_CALL(pool, make(_, _)).WillByDefault([this](const std::string& name,
            const std::vector<const expr*>&) -> const expr* {
            if (name == "g")
                return &pooled_g;
            if (name == "f")
                return &pooled_f;
            return nullptr;
        });
    }

    normalizer make_normalizer() {
        loc.bind_as<i_make_functor>(pool);
        loc.bind_as<i_bind_map>(bm);
        return normalizer{loc};
    }

    locator loc;
    NiceMock<MockBindMap> bm;
    NiceMock<MockExprPool> pool;

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr var_repr{expr::var{5}};
    expr f_raw{expr::functor{"f", {&var0}}};
    expr g_var0{expr::functor{"g", {&var0}}};
    expr f_g_var0{expr::functor{"f", {&g_var0}}};

    expr pooled_f{expr::functor{"f", {}}};
    expr pooled_g{expr::functor{"g", {}}};
    expr arg_whnf0{expr::var{90}};
    expr arg_whnf1{expr::var{91}};
    expr arg_whnf2{expr::var{92}};
};

// ---------------------------------------------------------------------------
// WHNF / variables
// ---------------------------------------------------------------------------

TEST_F(NormalizerUnitTest, VarAfterWhnfReturnsWhnfAndDoesNotCallFunctor) {
    ON_CALL(bm, whnf(&var0)).WillByDefault(Return(&var0));
    EXPECT_CALL(pool, make(_, _)).Times(0);

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize(&var0), &var0);
}

TEST_F(NormalizerUnitTest, BoundVarReturnsWhnfResult) {
    ON_CALL(bm, whnf(&var0)).WillByDefault(Return(&var_repr));
    EXPECT_CALL(pool, make(_, _)).Times(0);

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize(&var0), &var_repr);
}

// ---------------------------------------------------------------------------
// Functors
// ---------------------------------------------------------------------------

TEST_F(NormalizerUnitTest, FunctorRebuildsViaPool) {
    EXPECT_CALL(pool, make("f", ElementsAre(&arg_whnf0)))
        .WillOnce(Return(&pooled_f));

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize(&f_raw), &pooled_f);
}

TEST_F(NormalizerUnitTest, BinaryFunctorNormalizesBothArgs) {
    expr f2{expr::functor{"f", {&var0, &var1}}};

    EXPECT_CALL(pool, make("f", ElementsAre(&arg_whnf0, &arg_whnf1)))
        .WillOnce(Return(&pooled_f));

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize(&f2), &pooled_f);
}

TEST_F(NormalizerUnitTest, TernaryFunctorNormalizesAllArgs) {
    expr f3{expr::functor{"f", {&var0, &var1, &var2}}};

    EXPECT_CALL(pool, make("f", ElementsAre(&arg_whnf0, &arg_whnf1, &arg_whnf2)))
        .WillOnce(Return(&pooled_f));

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize(&f3), &pooled_f);
}

TEST_F(NormalizerUnitTest, NestedFunctorRebuildsWithNormalizedInnerArg) {
    EXPECT_CALL(pool, make("g", ElementsAre(&arg_whnf0)))
        .WillOnce(Return(&pooled_g));
    EXPECT_CALL(pool, make("f", ElementsAre(&pooled_g)))
        .WillOnce(Return(&pooled_f));

    normalizer norm = make_normalizer();
    EXPECT_EQ(norm.normalize(&f_g_var0), &pooled_f);
}
