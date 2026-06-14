#include <gtest/gtest.h>
#include <optional>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/trail.hpp"
#include "atom_fixture.hpp"

struct ExprPoolIntegrationTest : public ::testing::Test {
protected:
    test_atoms atoms;
    void SetUp() override {
        pool.emplace();
    }

    trail t;
    std::optional<expr_pool> pool;
};

TEST_F(ExprPoolIntegrationTest, SizeStartsAtZero) {
    EXPECT_EQ(pool->size(), 0u);
}

TEST_F(ExprPoolIntegrationTest, DistinctInternsIncreaseSize) {
    pool->make_var(0);
    EXPECT_EQ(pool->size(), 1u);
    pool->make_var(1);
    EXPECT_EQ(pool->size(), 2u);
}

TEST_F(ExprPoolIntegrationTest, RepeatInternDoesNotIncreaseSize) {
    pool->make_var(0);
    pool->make_var(0);
    EXPECT_EQ(pool->size(), 1u);
}

TEST_F(ExprPoolIntegrationTest, VarInternedBeforePushSurvivesPop) {
    const expr* p = pool->make_var(0);
    EXPECT_EQ(pool->size(), 1u);
    t.push();
    t.pop();
    EXPECT_EQ(pool->make_var(0), p);
    EXPECT_EQ(pool->size(), 1u);
}

TEST_F(ExprPoolIntegrationTest, PushPopWithoutInternLeavesSizeUnchanged) {
    pool->make_var(0);
    EXPECT_EQ(pool->size(), 1u);
    t.push();
    t.pop();
    EXPECT_EQ(pool->size(), 1u);
}

TEST_F(ExprPoolIntegrationTest, SizePersistsAfterInternInFrame) {
    pool->make_var(0);
    EXPECT_EQ(pool->size(), 1u);

    t.push();
    pool->make_var(1);
    EXPECT_EQ(pool->size(), 2u);
    t.pop();

    EXPECT_EQ(pool->size(), 2u);
}

TEST_F(ExprPoolIntegrationTest, ExprInternedBeforeFrameSurvivesPop) {
    const expr* p0 = pool->make_var(0);
    EXPECT_EQ(pool->size(), 1u);

    t.push();
    pool->make_var(1);
    EXPECT_EQ(pool->size(), 2u);
    t.pop();

    EXPECT_EQ(pool->make_var(0), p0);
    EXPECT_EQ(pool->size(), 2u);
}

TEST_F(ExprPoolIntegrationTest, MultipleInternsInFramePersistAfterPop) {
    t.push();
    pool->make_var(0);
    pool->make_var(1);
    pool->make_functor(atoms.id("f"), {});
    EXPECT_EQ(pool->size(), 3u);
    t.pop();
    EXPECT_EQ(pool->size(), 3u);
}

TEST_F(ExprPoolIntegrationTest, FunctorInternedInFramePersistsAfterPop) {
    const expr* v0 = pool->make_var(0);
    EXPECT_EQ(pool->size(), 1u);

    t.push();
    pool->make_functor(atoms.id("f"), {v0});
    EXPECT_EQ(pool->size(), 2u);
    t.pop();

    EXPECT_EQ(pool->size(), 2u);
    EXPECT_EQ(pool->make_var(0), v0);
}

TEST_F(ExprPoolIntegrationTest, TwoNestedFramesInnerPopRetainsAllInterns) {
    t.push();
    pool->make_var(0);
    EXPECT_EQ(pool->size(), 1u);

    t.push();
    pool->make_var(1);
    EXPECT_EQ(pool->size(), 2u);
    t.pop();
    EXPECT_EQ(pool->size(), 2u);

    t.pop();
    EXPECT_EQ(pool->size(), 2u);
}

TEST_F(ExprPoolIntegrationTest, TwoNestedFramesOuterExprSurvivesInnerPop) {
    t.push();
    const expr* p0 = pool->make_var(0);
    EXPECT_EQ(pool->size(), 1u);

    t.push();
    pool->make_var(1);
    EXPECT_EQ(pool->size(), 2u);
    t.pop();

    EXPECT_EQ(pool->make_var(0), p0);
    EXPECT_EQ(pool->size(), 2u);
}

TEST_F(ExprPoolIntegrationTest, ImportInFramePersistsAfterPop) {
    expr e{expr::var{7}};
    EXPECT_EQ(pool->size(), 0u);

    t.push();
    pool->import(&e);
    EXPECT_EQ(pool->size(), 1u);
    t.pop();

    EXPECT_EQ(pool->size(), 1u);
}

TEST_F(ExprPoolIntegrationTest, ImportFunctorInFramePersistsAfterPop) {
    expr arg{expr::var{1}};
    expr root{expr::functor{atoms.id("f"), {&arg}}};
    EXPECT_EQ(pool->size(), 0u);

    t.push();
    pool->import(&root);
    EXPECT_EQ(pool->size(), 2u);
    t.pop();

    EXPECT_EQ(pool->size(), 2u);
}
