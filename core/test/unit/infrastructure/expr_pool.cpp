// expr_pool canonicalizes expr nodes and logs first-seen internings to i_trail for
// backtracking. Unit tests mock the trail and assert pointer identity, trail logging
// counts, and import behavior without a real trail.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/expr_pool.hpp"
#include "../../../core/hpp/utility/i_trail.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::StrictMock;

struct MockTrail : public i_trail {
    MOCK_METHOD(void, push, (), (override));
    MOCK_METHOD(void, pop, (), (override));
    MOCK_METHOD(void, log, ((std::unique_ptr<i_backtrackable>)), (override));
};

struct ExprPoolTest : public ::testing::Test {
    NiceMock<MockTrail> trail;
    expr_pool pool{trail};
};

// ---------------------------------------------------------------------------
// Interning / identity
// ---------------------------------------------------------------------------

TEST_F(ExprPoolTest, VarInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make(0), pool.make(0));
}

TEST_F(ExprPoolTest, DifferentVarsReturnDifferentPointers) {
    EXPECT_NE(pool.make(0), pool.make(1));
}

TEST_F(ExprPoolTest, NullaryFunctorInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make("f", {}), pool.make("f", {}));
}

TEST_F(ExprPoolTest, DifferentFunctorsReturnDifferentPointers) {
    EXPECT_NE(pool.make("f", {}), pool.make("g", {}));
}

TEST_F(ExprPoolTest, FunctorWithVarArgInternedTwiceReturnsSamePointer) {
    const expr* v = pool.make(0);
    EXPECT_EQ(pool.make("f", {v}), pool.make("f", {v}));
}

TEST_F(ExprPoolTest, FunctorsSameNameDifferentArityReturnDifferentPointers) {
    const expr* v = pool.make(0);
    EXPECT_NE(pool.make("f", {}), pool.make("f", {v}));
}

TEST_F(ExprPoolTest, BinaryFunctorInternedTwiceReturnsSamePointer) {
    const expr* v0 = pool.make(0);
    const expr* v1 = pool.make(1);
    EXPECT_EQ(pool.make("f", {v0, v1}), pool.make("f", {v0, v1}));
}

TEST_F(ExprPoolTest, FunctorsWithDifferentArgsReturnDifferentPointers) {
    const expr* v0 = pool.make(0);
    const expr* v1 = pool.make(1);
    EXPECT_NE(pool.make("f", {v0, v1}), pool.make("f", {v1, v1}));
}

// ---------------------------------------------------------------------------
// Trail delegation (SUT → i_trail)
// ---------------------------------------------------------------------------

TEST_F(ExprPoolTest, FirstInternLogsToTrail) {
    StrictMock<MockTrail> strict_trail;
    expr_pool strict_pool{strict_trail};

    EXPECT_CALL(strict_trail, log(_)).Times(1);
    strict_pool.make(0);
}

TEST_F(ExprPoolTest, RepeatInternDoesNotLogAgain) {
    StrictMock<MockTrail> strict_trail;
    expr_pool strict_pool{strict_trail};

    EXPECT_CALL(strict_trail, log(_)).Times(1);
    strict_pool.make(0);
    strict_pool.make(0);
}

TEST_F(ExprPoolTest, DistinctInternsLogEachTime) {
    StrictMock<MockTrail> strict_trail;
    expr_pool strict_pool{strict_trail};

    EXPECT_CALL(strict_trail, log(_)).Times(2);
    strict_pool.make(0);
    strict_pool.make(1);
}

// ---------------------------------------------------------------------------
// import
// ---------------------------------------------------------------------------

TEST_F(ExprPoolTest, ImportVarInternsIntoPool) {
    StrictMock<MockTrail> strict_trail;
    expr_pool strict_pool{strict_trail};

    EXPECT_CALL(strict_trail, log(_)).Times(1);
    const expr* pooled = strict_pool.make(0);

    expr e{expr::var{0}};
    EXPECT_EQ(strict_pool.import(&e), pooled);
    EXPECT_EQ(strict_pool.import(&e), pooled);
}

TEST_F(ExprPoolTest, ImportFunctorInternsArgsRecursively) {
    StrictMock<MockTrail> strict_trail;
    expr_pool strict_pool{strict_trail};

    expr arg{expr::var{1}};
    expr root{expr::functor{"f", {&arg}}};

    EXPECT_CALL(strict_trail, log(_)).Times(2);
    const expr* p = strict_pool.import(&root);

    const expr::functor& f = std::get<expr::functor>(p->content);
    EXPECT_NE(f.args[0], &arg);
    EXPECT_EQ(f.args[0], strict_pool.make(1));
}
