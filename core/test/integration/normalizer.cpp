#include <gtest/gtest.h>
#include <optional>
#include "locator_fixture.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/trail.hpp"

struct NormalizerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        loc.bind_as<i_log_to_current_trail_frame>(t);
        pool.emplace(loc);
        loc.bind_as<i_make_functor>(*pool);
        loc.bind_as<i_bind_map>(bm);
        norm.emplace(loc);
    }

    locator loc;
    trail t;
    bind_map bm;
    std::optional<expr_pool> pool;
    std::optional<normalizer> norm;

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};

    const expr* pool_f() { return pool->make("f", {}); }
    const expr* pool_g() { return pool->make("g", {}); }
};

// ---------------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------------

TEST_F(NormalizerIntegrationTest, NormalizeUnboundVarReturnsWhnfSelf) {
    const expr* p = norm->normalize(&var0);
    EXPECT_EQ(p, &var0);
    EXPECT_EQ(std::get<expr::var>(p->content).index, 0u);
}

TEST_F(NormalizerIntegrationTest, NormalizeBoundVarReturnsWhnfRepresentative) {
    const expr* f = pool_f();
    bm.bind(0, f);
    EXPECT_EQ(norm->normalize(&var0), f);
}

// ---------------------------------------------------------------------------
// Functors
// ---------------------------------------------------------------------------

TEST_F(NormalizerIntegrationTest, NormalizeNullaryFunctorInternsInPool) {
    expr raw{expr::functor{"f", {}}};
    const expr* p1 = norm->normalize(&raw);
    const expr* p2 = norm->normalize(&raw);
    EXPECT_EQ(p1, p2);
    EXPECT_NE(p1, &raw);
    EXPECT_EQ(p1, pool->make("f", {}));
}

TEST_F(NormalizerIntegrationTest, NormalizeFunctorNormalizesArgs) {
    const expr* f = pool_f();
    expr f_var0{expr::functor{"f", {&var0}}};
    bm.bind(0, f);
    const expr* p = norm->normalize(&f_var0);
    const expr::functor& out = std::get<expr::functor>(p->content);
    EXPECT_EQ(out.args[0], f);
}

TEST_F(NormalizerIntegrationTest, NormalizeNestedFunctorRebuildsStructure) {
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
    EXPECT_EQ(p, pool->make("f", {pool->make("g", {g})}));
}

TEST_F(NormalizerIntegrationTest, NormalizeBinaryFunctorNormalizesBothArgs) {
    const expr* a = pool->make("a", {});
    const expr* b = pool->make("b", {});
    expr raw{expr::functor{"f", {&var0, &var1}}};
    bm.bind(0, a);
    bm.bind(1, b);
    const expr* p = norm->normalize(&raw);
    const expr::functor& out = std::get<expr::functor>(p->content);
    ASSERT_EQ(out.args.size(), 2u);
    EXPECT_EQ(out.args[0], a);
    EXPECT_EQ(out.args[1], b);
    EXPECT_EQ(p, pool->make("f", {a, b}));
}

TEST_F(NormalizerIntegrationTest, NormalizeTernaryFunctorNormalizesAllArgs) {
    const expr* a = pool->make("a", {});
    const expr* b = pool->make("b", {});
    const expr* c = pool->make("c", {});
    expr raw{expr::functor{"f", {&var0, &var1, &var2}}};
    bm.bind(0, a);
    bm.bind(1, b);
    bm.bind(2, c);
    const expr* p = norm->normalize(&raw);
    const expr::functor& out = std::get<expr::functor>(p->content);
    ASSERT_EQ(out.args.size(), 3u);
    EXPECT_EQ(out.args[0], a);
    EXPECT_EQ(out.args[1], b);
    EXPECT_EQ(out.args[2], c);
    EXPECT_EQ(p, pool->make("f", {a, b, c}));
}

TEST_F(NormalizerIntegrationTest, NormalizeIdempotentOnAlreadyNormalized) {
    expr raw{expr::functor{"f", {}}};
    const expr* p1 = norm->normalize(&raw);
    EXPECT_EQ(norm->normalize(p1), p1);
}
