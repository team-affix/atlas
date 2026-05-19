#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/normalizer.hpp"
#include "../../../core/hpp/interfaces/i_bind_map.hpp"
#include "../../../core/hpp/interfaces/i_expr_pool.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class MockBindMap : public i_bind_map {
public:
    MOCK_METHOD(void,        bind, (uint32_t, const expr*), (override));
    MOCK_METHOD(const expr*, whnf, (const expr*),           (override));
};

class MockExprPool : public i_expr_pool {
public:
    MOCK_METHOD(const expr*, functor, (const std::string&, std::vector<const expr*>), (override));
    MOCK_METHOD(const expr*, var, (uint32_t), (override));
    MOCK_METHOD(const expr*, import, (const expr*), (override));
};

class NormalizerUnitTest : public ::testing::Test {
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
    std::vector<const expr*> captured_args;

    ON_CALL(pool, functor("f", _))
        .WillByDefault([&](const std::string&, const std::vector<const expr*>& args) {
            captured_args = args;
            return &pooled_f;
        });

    EXPECT_EQ(norm.normalize(&f_raw), &pooled_f);
    ASSERT_EQ(captured_args.size(), 1u);
    EXPECT_EQ(captured_args[0], &arg_whnf0);
}

TEST_F(NormalizerUnitTest, BinaryFunctorNormalizesBothArgs) {
    expr f2{expr::functor{"f", {&var0, &var1}}};
    std::vector<const expr*> captured_args;

    ON_CALL(pool, functor("f", _))
        .WillByDefault([&](const std::string&, const std::vector<const expr*>& args) {
            captured_args = args;
            return &pooled_f;
        });

    EXPECT_EQ(norm.normalize(&f2), &pooled_f);
    ASSERT_EQ(captured_args.size(), 2u);
    EXPECT_EQ(captured_args[0], &arg_whnf0);
    EXPECT_EQ(captured_args[1], &arg_whnf1);
}

TEST_F(NormalizerUnitTest, TernaryFunctorNormalizesAllArgs) {
    expr f3{expr::functor{"f", {&var0, &var1, &var2}}};
    std::vector<const expr*> captured_args;

    ON_CALL(pool, functor("f", _))
        .WillByDefault([&](const std::string&, const std::vector<const expr*>& args) {
            captured_args = args;
            return &pooled_f;
        });

    EXPECT_EQ(norm.normalize(&f3), &pooled_f);
    ASSERT_EQ(captured_args.size(), 3u);
    EXPECT_EQ(captured_args[0], &arg_whnf0);
    EXPECT_EQ(captured_args[1], &arg_whnf1);
    EXPECT_EQ(captured_args[2], &arg_whnf2);
}

TEST_F(NormalizerUnitTest, NestedFunctorRebuildsWithNormalizedInnerArg) {
    std::vector<const expr*> captured_f_args;

    ON_CALL(pool, functor("f", _))
        .WillByDefault([&](const std::string&, const std::vector<const expr*>& args) {
            captured_f_args = args;
            return &pooled_f;
        });

    EXPECT_EQ(norm.normalize(&f_g_var0), &pooled_f);
    ASSERT_EQ(captured_f_args.size(), 1u);
    EXPECT_EQ(captured_f_args[0], &pooled_g);
}
