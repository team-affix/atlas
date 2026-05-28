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

TEST_F(ExprPoolIntegrationTest, SizeStartsAtZero) {
    EXPECT_EQ(pool->size(), 0u);
}

TEST_F(ExprPoolIntegrationTest, DistinctInternsIncreaseSize) {
    pool->make(0);
    EXPECT_EQ(pool->size(), 1u);
    pool->make(1);
    EXPECT_EQ(pool->size(), 2u);
}

TEST_F(ExprPoolIntegrationTest, RepeatInternDoesNotIncreaseSize) {
    pool->make(0);
    pool->make(0);
    EXPECT_EQ(pool->size(), 1u);
}

TEST_F(ExprPoolIntegrationTest, VarInternedBeforePushSurvivesPop) {
    const expr* p = pool->make(0);
    EXPECT_EQ(pool->size(), 1u);
    t.push();
    t.pop();
    EXPECT_EQ(pool->make(0), p);
    EXPECT_EQ(pool->size(), 1u);
}

TEST_F(ExprPoolIntegrationTest, PushPopWithoutInternLeavesSizeUnchanged) {
    pool->make(0);
    EXPECT_EQ(pool->size(), 1u);
    t.push();
    t.pop();
    EXPECT_EQ(pool->size(), 1u);
}

TEST_F(ExprPoolIntegrationTest, SizeRevertsAfterInternInFrame) {
    pool->make(0);
    EXPECT_EQ(pool->size(), 1u);

    t.push();
    pool->make(1);
    EXPECT_EQ(pool->size(), 2u);
    t.pop();

    EXPECT_EQ(pool->size(), 1u);
}

TEST_F(ExprPoolIntegrationTest, ExprInternedBeforeFrameSurvivesPop) {
    const expr* p0 = pool->make(0);
    EXPECT_EQ(pool->size(), 1u);

    t.push();
    pool->make(1);
    EXPECT_EQ(pool->size(), 2u);
    t.pop();

    EXPECT_EQ(pool->make(0), p0);
    EXPECT_EQ(pool->size(), 1u);
}

TEST_F(ExprPoolIntegrationTest, MultipleInternsInFrameAllRevertOnPop) {
    t.push();
    pool->make(0);
    pool->make(1);
    pool->make("f", {});
    EXPECT_EQ(pool->size(), 3u);
    t.pop();
    EXPECT_EQ(pool->size(), 0u);
}

TEST_F(ExprPoolIntegrationTest, FunctorInternedInFrameRevertsButArgsSurvive) {
    const expr* v0 = pool->make(0);
    EXPECT_EQ(pool->size(), 1u);

    t.push();
    pool->make("f", {v0});
    EXPECT_EQ(pool->size(), 2u);
    t.pop();

    EXPECT_EQ(pool->size(), 1u);
    EXPECT_EQ(pool->make(0), v0);
}

TEST_F(ExprPoolIntegrationTest, TwoNestedFramesInnerPopRevertsOnlyInnerGrowth) {
    t.push();
    pool->make(0);
    EXPECT_EQ(pool->size(), 1u);

    t.push();
    pool->make(1);
    EXPECT_EQ(pool->size(), 2u);
    t.pop();
    EXPECT_EQ(pool->size(), 1u);

    t.pop();
    EXPECT_EQ(pool->size(), 0u);
}

TEST_F(ExprPoolIntegrationTest, TwoNestedFramesOuterExprSurvivesInnerPop) {
    t.push();
    const expr* p0 = pool->make(0);
    EXPECT_EQ(pool->size(), 1u);

    t.push();
    pool->make(1);
    EXPECT_EQ(pool->size(), 2u);
    t.pop();

    EXPECT_EQ(pool->make(0), p0);
    EXPECT_EQ(pool->size(), 1u);
}

TEST_F(ExprPoolIntegrationTest, ImportInFrameRevertsSize) {
    expr e{expr::var{7}};
    EXPECT_EQ(pool->size(), 0u);

    t.push();
    pool->import(&e);
    EXPECT_EQ(pool->size(), 1u);
    t.pop();

    EXPECT_EQ(pool->size(), 0u);
}

TEST_F(ExprPoolIntegrationTest, ImportFunctorInFrameRevertsAllNodes) {
    expr arg{expr::var{1}};
    expr root{expr::functor{"f", {&arg}}};
    EXPECT_EQ(pool->size(), 0u);

    t.push();
    pool->import(&root);
    EXPECT_EQ(pool->size(), 2u);
    t.pop();

    EXPECT_EQ(pool->size(), 0u);
}
