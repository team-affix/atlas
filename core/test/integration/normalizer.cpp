#include <gtest/gtest.h>
#include <optional>
#include "infrastructure/normalizer.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/trail.hpp"
#include "functor_fixture.hpp"

using TestNormalizer = normalizer<globalizer, expr_pool, expr_pool, bind_map>;

struct NormalizerIntegrationTest : public ::testing::Test {
protected:
    test_functors functors;
    void SetUp() override {
        pool.emplace();
        glob.emplace();
        bm.emplace(*glob);
        norm.emplace(*glob, *pool, *pool, *bm);
    }

    trail t;
    std::optional<globalizer> glob;
    std::optional<bind_map> bm;
    std::optional<expr_pool> pool;
    std::optional<TestNormalizer> norm;

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};

    const expr* pool_f() { return pool->make_functor(functors.id("f"), {}); }
    const expr* pool_g() { return pool->make_functor(functors.id("g"), {}); }

    // Normalize with global frame (initial goals live at offset 0).
    const expr* norm0(const expr* e) { return norm->normalize({e, 0}); }
};

// ---------------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------------

TEST_F(NormalizerIntegrationTest, NormalizeUnboundVarReturnsWhnfSelf) {
    const expr* p = norm0(&var0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(std::get<expr::var>(p->content).index, 0u);
}

TEST_F(NormalizerIntegrationTest, NormalizeBoundVarReturnsWhnfRepresentative) {
    const expr* f = pool_f();
    bm->bind(0, {f, 0});
    EXPECT_EQ(norm0(&var0), f);
}

// ---------------------------------------------------------------------------
// Functors
// ---------------------------------------------------------------------------

TEST_F(NormalizerIntegrationTest, NormalizeNullaryFunctorInternsInPool) {
    expr raw{expr::functor{functors.id("f"), {}}};
    const expr* p1 = norm0(&raw);
    const expr* p2 = norm0(&raw);
    EXPECT_EQ(p1, p2);
    EXPECT_NE(p1, &raw);
    EXPECT_EQ(p1, pool->make_functor(functors.id("f"), {}));
}

TEST_F(NormalizerIntegrationTest, NormalizeFunctorNormalizesArgs) {
    const expr* f = pool_f();
    expr f_var0{expr::functor{functors.id("f"), {&var0}}};
    bm->bind(0, {f, 0});
    const expr* p = norm0(&f_var0);
    const expr::functor& out = std::get<expr::functor>(p->content);
    EXPECT_EQ(out.args[0], f);
}

TEST_F(NormalizerIntegrationTest, NormalizeNestedFunctorRebuildsStructure) {
    const expr* f = pool_f();
    const expr* g = pool_g();
    expr g_var0{expr::functor{functors.id("g"), {&var0}}};
    expr outer_raw{expr::functor{functors.id("f"), {&g_var0}}};
    bm->bind(0, {g, 0});
    const expr* p = norm0(&outer_raw);
    const expr::functor& outer = std::get<expr::functor>(p->content);
    const expr::functor& inner = std::get<expr::functor>(outer.args[0]->content);
    EXPECT_EQ(outer.id, functors.id("f"));
    EXPECT_EQ(inner.id, functors.id("g"));
    EXPECT_EQ(inner.args[0], g);
    EXPECT_EQ(p, pool->make_functor(functors.id("f"), {pool->make_functor(functors.id("g"), {g})}));
}

TEST_F(NormalizerIntegrationTest, NormalizeBinaryFunctorNormalizesBothArgs) {
    const expr* a = pool->make_functor(functors.id("a"), {});
    const expr* b = pool->make_functor(functors.id("b"), {});
    expr raw{expr::functor{functors.id("f"), {&var0, &var1}}};
    bm->bind(0, {a, 0});
    bm->bind(1, {b, 0});
    const expr* p = norm0(&raw);
    const expr::functor& out = std::get<expr::functor>(p->content);
    ASSERT_EQ(out.args.size(), 2u);
    EXPECT_EQ(out.args[0], a);
    EXPECT_EQ(out.args[1], b);
    EXPECT_EQ(p, pool->make_functor(functors.id("f"), {a, b}));
}

TEST_F(NormalizerIntegrationTest, NormalizeTernaryFunctorNormalizesAllArgs) {
    const expr* a = pool->make_functor(functors.id("a"), {});
    const expr* b = pool->make_functor(functors.id("b"), {});
    const expr* c = pool->make_functor(functors.id("c"), {});
    expr raw{expr::functor{functors.id("f"), {&var0, &var1, &var2}}};
    bm->bind(0, {a, 0});
    bm->bind(1, {b, 0});
    bm->bind(2, {c, 0});
    const expr* p = norm0(&raw);
    const expr::functor& out = std::get<expr::functor>(p->content);
    ASSERT_EQ(out.args.size(), 3u);
    EXPECT_EQ(out.args[0], a);
    EXPECT_EQ(out.args[1], b);
    EXPECT_EQ(out.args[2], c);
    EXPECT_EQ(p, pool->make_functor(functors.id("f"), {a, b, c}));
}

TEST_F(NormalizerIntegrationTest, NormalizeIdempotentOnAlreadyNormalized) {
    expr raw{expr::functor{functors.id("f"), {}}};
    const expr* p1 = norm0(&raw);
    EXPECT_EQ(norm0(p1), p1);
}
