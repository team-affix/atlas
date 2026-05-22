#include <gtest/gtest.h>
#include <optional>
#include "../../../core/hpp/infrastructure/expr_pool.hpp"
#include "../../../core/hpp/utility/trail.hpp"

struct ExprPoolIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override { pool.emplace(t); }

    trail t;
    std::optional<expr_pool> pool;
};

TEST_F(ExprPoolIntegrationTest, VarInternedBeforePushSurvivesPop) {
    const expr* p = pool->var(0);
    t.push();
    t.pop();
    EXPECT_EQ(pool->var(0), p);
}

TEST_F(ExprPoolIntegrationTest, ExprInternedBeforeFrameSurvivesPop) {
    const expr* p0 = pool->var(0);
    t.push();
    pool->var(1);
    t.pop();
    EXPECT_EQ(pool->var(0), p0);
}

TEST_F(ExprPoolIntegrationTest, TwoNestedFramesOuterExprSurvivesInnerPop) {
    t.push();
    const expr* p0 = pool->var(0);
    t.push();
    pool->var(1);
    t.pop();
    EXPECT_EQ(pool->var(0), p0);
}
