#include <gtest/gtest.h>
#include <optional>
#include "../../../core/hpp/infrastructure/copier.hpp"
#include "../../../core/hpp/infrastructure/expr_pool.hpp"
#include "../../../core/hpp/infrastructure/var_sequencer.hpp"
#include "../../../core/hpp/utility/trail.hpp"

class CopierTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool.emplace(t);
        cp.emplace(vs, *pool);
    }

    trail t;
    var_sequencer vs{t};
    std::optional<expr_pool> pool;
    std::optional<copier> cp;

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
};

static uint32_t var_index(const expr* e) {
    return std::get<expr::var>(e->content).index;
}

// ---------------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------------

TEST_F(CopierTest, CopyVarAllocatesFreshIndex) {
    std::unordered_map<uint32_t, uint32_t> map;
    const expr* p = cp->copy(&var0, map);
    EXPECT_EQ(var_index(p), 0u);
    EXPECT_EQ(map.at(0), 0u);
}

TEST_F(CopierTest, CopySameVarTwiceReusesMappedIndex) {
    std::unordered_map<uint32_t, uint32_t> map;
    const expr* p1 = cp->copy(&var0, map);
    const expr* p2 = cp->copy(&var0, map);
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(map.size(), 1u);
}

TEST_F(CopierTest, CopyDistinctVarsGetDistinctIndices) {
    std::unordered_map<uint32_t, uint32_t> map;
    const expr* p0 = cp->copy(&var0, map);
    const expr* p1 = cp->copy(&var1, map);
    EXPECT_EQ(var_index(p0), 0u);
    EXPECT_EQ(var_index(p1), 1u);
}

// ---------------------------------------------------------------------------
// Functors
// ---------------------------------------------------------------------------

TEST_F(CopierTest, CopyFunctorPreservesNameAndArity) {
    expr f{expr::functor{"f", {&var0, &var1}}};
    std::unordered_map<uint32_t, uint32_t> map;
    const expr* p = cp->copy(&f, map);
    const expr::functor& out = std::get<expr::functor>(p->content);
    EXPECT_EQ(out.name, "f");
    EXPECT_EQ(out.args.size(), 2u);
}

TEST_F(CopierTest, CopyNestedFunctorRemapsAllVars) {
    expr inner{expr::functor{"g", {&var0}}};
    expr outer{expr::functor{"f", {&inner, &var1}}};
    std::unordered_map<uint32_t, uint32_t> map;
    const expr* p = cp->copy(&outer, map);
    const expr::functor& f = std::get<expr::functor>(p->content);
    const expr::functor& g = std::get<expr::functor>(f.args[0]->content);
    EXPECT_EQ(var_index(g.args[0]), 0u);
    EXPECT_EQ(var_index(f.args[1]), 1u);
    EXPECT_EQ(map.at(0), 0u);
    EXPECT_EQ(map.at(1), 1u);
}

TEST_F(CopierTest, CopyTernaryFunctorRemapsAllVars) {
    expr f{expr::functor{"f", {&var0, &var1, &var2}}};
    std::unordered_map<uint32_t, uint32_t> map;
    const expr* p = cp->copy(&f, map);
    const expr::functor& out = std::get<expr::functor>(p->content);
    EXPECT_EQ(out.name, "f");
    ASSERT_EQ(out.args.size(), 3u);
    EXPECT_EQ(var_index(out.args[0]), 0u);
    EXPECT_EQ(var_index(out.args[1]), 1u);
    EXPECT_EQ(var_index(out.args[2]), 2u);
    EXPECT_EQ(map.size(), 3u);
}

TEST_F(CopierTest, CopySeparateCallsAdvanceSequencer) {
    std::unordered_map<uint32_t, uint32_t> map1;
    std::unordered_map<uint32_t, uint32_t> map2;
    const expr* p1 = cp->copy(&var0, map1);
    const expr* p2 = cp->copy(&var0, map2);
    EXPECT_EQ(var_index(p1), 0u);
    EXPECT_EQ(var_index(p2), 1u);
}
