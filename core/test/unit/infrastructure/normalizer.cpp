// normalizer rebuilds expressions in WHNF via i_expr_pool after resolving variables
// through i_bind_map. Unit tests mock both interfaces and assert pool calls use WHNF args.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/normalizer.hpp"
#include "../../../core/hpp/interfaces/i_bind_map.hpp"
#include "../../../core/hpp/interfaces/i_expr_pool.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;

struct MockBindMap : public i_bind_map {
    MOCK_METHOD(void,        bind, (uint32_t, const expr*), (override));
    MOCK_METHOD(const expr*, whnf, (const expr*),           (override));
};

struct MockExprPool : public i_expr_pool {
    MOCK_METHOD(const expr*, functor, (const std::string&, std::vector<const expr*>), (override));
    MOCK_METHOD(const expr*, var, (uint32_t), (override));
    MOCK_METHOD(const expr*, import, (const expr*), (override));
    MOCK_METHOD(size_t, size, (), (const, override));
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
        ON_CALL(pool, functor(_, _)).WillByDefault([this](const std::string& name,
            const std::vector<const expr*>&) -> const expr* {
            if (name == "g")
                return &pooled_g;
            if (name == "f")
                return &pooled_f;
            return nullptr;
        });
    }

    NiceMock<MockBindMap> bm;
    NiceMock<MockExprPool> pool;
    normalizer norm{pool, bm};

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

    EXPECT_CALL(pool, functor).Times(0);

    EXPECT_EQ(norm.normalize(&var0), &var0);
}

TEST_F(NormalizerUnitTest, BoundVarReturnsWhnfResult) {
    ON_CALL(bm, whnf(&var0)).WillByDefault(Return(&var_repr));

    EXPECT_CALL(pool, functor).Times(0);

    EXPECT_EQ(norm.normalize(&var0), &var_repr);
}

// ---------------------------------------------------------------------------
// Functors
// ---------------------------------------------------------------------------

TEST_F(NormalizerUnitTest, FunctorRebuildsViaPool) {
    EXPECT_CALL(pool, functor("f", ElementsAre(&arg_whnf0)))
        .WillOnce(Return(&pooled_f));

    EXPECT_EQ(norm.normalize(&f_raw), &pooled_f);
}

TEST_F(NormalizerUnitTest, BinaryFunctorNormalizesBothArgs) {
    expr f2{expr::functor{"f", {&var0, &var1}}};

    EXPECT_CALL(pool, functor("f", ElementsAre(&arg_whnf0, &arg_whnf1)))
        .WillOnce(Return(&pooled_f));

    EXPECT_EQ(norm.normalize(&f2), &pooled_f);
}

TEST_F(NormalizerUnitTest, TernaryFunctorNormalizesAllArgs) {
    expr f3{expr::functor{"f", {&var0, &var1, &var2}}};

    EXPECT_CALL(pool, functor("f", ElementsAre(&arg_whnf0, &arg_whnf1, &arg_whnf2)))
        .WillOnce(Return(&pooled_f));

    EXPECT_EQ(norm.normalize(&f3), &pooled_f);
}

TEST_F(NormalizerUnitTest, NestedFunctorRebuildsWithNormalizedInnerArg) {
    EXPECT_CALL(pool, functor("g", ElementsAre(&arg_whnf0)))
        .WillOnce(Return(&pooled_g));
    EXPECT_CALL(pool, functor("f", ElementsAre(&pooled_g)))
        .WillOnce(Return(&pooled_f));

    EXPECT_EQ(norm.normalize(&f_g_var0), &pooled_f);
}
