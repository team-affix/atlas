#include <gtest/gtest.h>
#include <optional>
#include "../../../core/hpp/infrastructure/normalizer.hpp"
#include "../../../core/hpp/infrastructure/expr_pool.hpp"
#include "../../../core/hpp/infrastructure/bind_map.hpp"
#include "../../../core/hpp/utility/trail.hpp"

class NormalizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool.emplace(t);
        norm.emplace(*pool, bm);
    }

    trail t;
    bind_map bm;
    std::optional<expr_pool> pool;
    std::optional<normalizer> norm;

    expr var0{expr::var{0}};

    const expr* pool_f() { return pool->functor("f", {}); }
    const expr* pool_g() { return pool->functor("g", {}); }
};

// ---------------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------------

TEST_F(NormalizerTest, NormalizeUnboundVarReturnsWhnfSelf) {
    const expr* p = norm->normalize(&var0);
    EXPECT_EQ(p, &var0);
    EXPECT_EQ(std::get<expr::var>(p->content).index, 0u);
}

TEST_F(NormalizerTest, NormalizeBoundVarReturnsWhnfRepresentative) {
    const expr* f = pool_f();
    bm.bind(0, f);
    EXPECT_EQ(norm->normalize(&var0), f);
}

// ---------------------------------------------------------------------------
// Functors
// ---------------------------------------------------------------------------

TEST_F(NormalizerTest, NormalizeNullaryFunctorInternsInPool) {
    expr raw{expr::functor{"f", {}}};
    const expr* p1 = norm->normalize(&raw);
    const expr* p2 = norm->normalize(&raw);
    EXPECT_EQ(p1, p2);
    EXPECT_NE(p1, &raw);
    EXPECT_EQ(p1, pool->functor("f", {}));
}

TEST_F(NormalizerTest, NormalizeFunctorNormalizesArgs) {
    const expr* f = pool_f();
    expr f_var0{expr::functor{"f", {&var0}}};
    bm.bind(0, f);
    const expr* p = norm->normalize(&f_var0);
    const expr::functor& out = std::get<expr::functor>(p->content);
    EXPECT_EQ(out.args[0], f);
}

TEST_F(NormalizerTest, NormalizeNestedFunctorRebuildsStructure) {
    const expr* f = pool_f();
    const expr* g = pool_g();
    expr g_var0{expr::functor{"g", {&var0}}};
    expr outer_raw{expr::functor{"f", {&g_var0}}};
    bm.bind(0, g);
    const expr* p = norm->normalize(&outer_raw);
    const expr::functor& outer = std::get<expr::functor>(p->content);
    const expr::functor& inner = std::get<expr::functor>(outer.args[0]->content);
    EXPECT_EQ(outer.name, "f");
    EXPECT_EQ(inner.name, "g");
    EXPECT_EQ(inner.args[0], g);
    EXPECT_EQ(p, pool->functor("f", {pool->functor("g", {g})}));
}

TEST_F(NormalizerTest, NormalizeIdempotentOnAlreadyNormalized) {
    expr raw{expr::functor{"f", {}}};
    const expr* p1 = norm->normalize(&raw);
    EXPECT_EQ(norm->normalize(p1), p1);
}
