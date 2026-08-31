// var_compactor: renumbers globalized variable indices to contiguous 0..K-1.
// Uses real expr_pool as IMakeFunctor/IMakeVar (dumb allocator, no behavior
// under test). The SUT is var_compactor.

#include <unordered_map>
#include <gtest/gtest.h>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/var_compactor.hpp"
#include "functor_fixture.hpp"

using sut_t = var_compactor<expr_pool, expr_pool>;

struct VarCompactorTest : public ::testing::Test {
    test_functors functors;
    expr_pool     pool;
    sut_t         sut{pool, pool};

    const expr* compact(const expr* e,
                        std::unordered_map<uint32_t, uint32_t>& var_map,
                        uint32_t& next_idx) {
        return sut.compact_vars(e, var_map, next_idx);
    }

    static uint32_t var_index(const expr* e) {
        return std::get<expr::var>(e->content).index;
    }

    static uint32_t functor_id(const expr* e) {
        return std::get<expr::functor>(e->content).id;
    }
};

// ---------------------------------------------------------------------------
// Nullary functor — no variables; next_idx unchanged
// ---------------------------------------------------------------------------

TEST_F(VarCompactorTest, NullaryFunctorProducedUnchanged) {
    const expr* f = pool.make_functor(functors.id("f"), {});
    std::unordered_map<uint32_t, uint32_t> var_map;
    uint32_t next_idx = 0;

    const expr* result = compact(f, var_map, next_idx);

    EXPECT_EQ(functor_id(result), functors.id("f"));
    EXPECT_TRUE(std::get<expr::functor>(result->content).args.empty());
    EXPECT_EQ(next_idx, 0u);
}

// ---------------------------------------------------------------------------
// Single variable remapping
// ---------------------------------------------------------------------------

TEST_F(VarCompactorTest, VarAtIndexZeroMapsToZero) {
    const expr* v = pool.make_var(0);
    std::unordered_map<uint32_t, uint32_t> var_map;
    uint32_t next_idx = 0;

    const expr* result = compact(v, var_map, next_idx);

    EXPECT_EQ(var_index(result), 0u);
    EXPECT_EQ(next_idx, 1u);
}

TEST_F(VarCompactorTest, VarAtHighIndexRemappedToZero) {
    const expr* v = pool.make_var(42);
    std::unordered_map<uint32_t, uint32_t> var_map;
    uint32_t next_idx = 0;

    const expr* result = compact(v, var_map, next_idx);

    EXPECT_EQ(var_index(result), 0u);
    EXPECT_EQ(next_idx, 1u);
}

// ---------------------------------------------------------------------------
// Two distinct variables
// ---------------------------------------------------------------------------

TEST_F(VarCompactorTest, TwoDistinctVarsInOrderRemapped) {
    expr raw{expr::functor{functors.id("f"), {pool.make_var(5), pool.make_var(10)}}};
    std::unordered_map<uint32_t, uint32_t> var_map;
    uint32_t next_idx = 0;

    const expr* result = compact(&raw, var_map, next_idx);

    const auto& args = std::get<expr::functor>(result->content).args;
    ASSERT_EQ(args.size(), 2u);
    EXPECT_EQ(var_index(args[0]), 0u);
    EXPECT_EQ(var_index(args[1]), 1u);
    EXPECT_EQ(next_idx, 2u);
}

TEST_F(VarCompactorTest, VarOrderDrivenByFirstAppearance) {
    // var(10) appears first so it gets 0; var(5) gets 1
    expr raw{expr::functor{functors.id("f"), {pool.make_var(10), pool.make_var(5)}}};
    std::unordered_map<uint32_t, uint32_t> var_map;
    uint32_t next_idx = 0;

    const expr* result = compact(&raw, var_map, next_idx);

    const auto& args = std::get<expr::functor>(result->content).args;
    ASSERT_EQ(args.size(), 2u);
    EXPECT_EQ(var_index(args[0]), 0u);
    EXPECT_EQ(var_index(args[1]), 1u);
    EXPECT_EQ(next_idx, 2u);
}

// ---------------------------------------------------------------------------
// Same variable appearing twice — same compacted index both times
// ---------------------------------------------------------------------------

TEST_F(VarCompactorTest, SameVarTwiceGivesSameIndex) {
    expr raw{expr::functor{functors.id("f"), {pool.make_var(7), pool.make_var(7)}}};
    std::unordered_map<uint32_t, uint32_t> var_map;
    uint32_t next_idx = 0;

    const expr* result = compact(&raw, var_map, next_idx);

    const auto& args = std::get<expr::functor>(result->content).args;
    ASSERT_EQ(args.size(), 2u);
    EXPECT_EQ(var_index(args[0]), 0u);
    EXPECT_EQ(var_index(args[1]), 0u);
    EXPECT_EQ(next_idx, 1u);
}

// ---------------------------------------------------------------------------
// Nested functor — compaction propagates through the tree
// ---------------------------------------------------------------------------

TEST_F(VarCompactorTest, NestedFunctorVarsPropagated) {
    const expr* inner = pool.make_functor(functors.id("f"), {pool.make_var(9)});
    const expr* outer_raw = pool.make_functor(functors.id("g"), {inner});
    std::unordered_map<uint32_t, uint32_t> var_map;
    uint32_t next_idx = 0;

    const expr* result = compact(outer_raw, var_map, next_idx);

    const auto& outer_args = std::get<expr::functor>(result->content).args;
    ASSERT_EQ(outer_args.size(), 1u);
    const auto& inner_args = std::get<expr::functor>(outer_args[0]->content).args;
    ASSERT_EQ(inner_args.size(), 1u);
    EXPECT_EQ(var_index(inner_args[0]), 0u);
    EXPECT_EQ(next_idx, 1u);
}

// ---------------------------------------------------------------------------
// Shared map across two calls — same var seen again resolves consistently
// ---------------------------------------------------------------------------

TEST_F(VarCompactorTest, SharedMapAcrossMultipleCalls) {
    const expr* v = pool.make_var(5);
    std::unordered_map<uint32_t, uint32_t> var_map;
    uint32_t next_idx = 0;

    const expr* r1 = compact(v, var_map, next_idx);
    const expr* r2 = compact(v, var_map, next_idx);

    EXPECT_EQ(var_index(r1), 0u);
    EXPECT_EQ(var_index(r2), 0u);
    EXPECT_EQ(next_idx, 1u);
}

// ---------------------------------------------------------------------------
// Fresh map — new compaction pass starts at 0 regardless of previous
// ---------------------------------------------------------------------------

TEST_F(VarCompactorTest, FreshMapStartsAgainAtZero) {
    const expr* v = pool.make_var(5);

    std::unordered_map<uint32_t, uint32_t> map1;
    uint32_t idx1 = 0;
    const expr* r1 = compact(v, map1, idx1);

    std::unordered_map<uint32_t, uint32_t> map2;
    uint32_t idx2 = 0;
    const expr* r2 = compact(v, map2, idx2);

    EXPECT_EQ(var_index(r1), 0u);
    EXPECT_EQ(var_index(r2), 0u);
}
