// expr_pool canonicalizes expr nodes and logs first-seen internings to
// i_log_to_current_trail_frame for
// backtracking. Unit tests mock the trail and assert pointer identity, trail logging
// counts, and import behavior without a real trail.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::StrictMock;

struct MockTrail : public i_log_to_current_trail_frame {
    MOCK_METHOD(void, log, ((std::unique_ptr<i_backtrackable>)), (override));
};

struct ExprPoolTest : public ::testing::Test {
    NiceMock<MockTrail> trail;
    locator loc;
    expr_pool pool;

    ExprPoolTest()
        : pool(bind_and_make<expr_pool, i_log_to_current_trail_frame>(loc, trail)) {}
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
// Trail delegation (SUT → i_log_to_current_trail_frame)
// ---------------------------------------------------------------------------

TEST_F(ExprPoolTest, FirstInternLogsToTrail) {
    StrictMock<MockTrail> strict_trail;
    locator loc;
    loc.bind_as<i_log_to_current_trail_frame>(strict_trail);
    expr_pool strict_pool{loc};

    EXPECT_CALL(strict_trail, log(_)).Times(1);
    strict_pool.make(0);
}

TEST_F(ExprPoolTest, RepeatInternDoesNotLogAgain) {
    StrictMock<MockTrail> strict_trail;
    locator loc;
    loc.bind_as<i_log_to_current_trail_frame>(strict_trail);
    expr_pool strict_pool{loc};

    EXPECT_CALL(strict_trail, log(_)).Times(1);
    strict_pool.make(0);
    strict_pool.make(0);
}

TEST_F(ExprPoolTest, DistinctInternsLogEachTime) {
    StrictMock<MockTrail> strict_trail;
    locator loc;
    loc.bind_as<i_log_to_current_trail_frame>(strict_trail);
    expr_pool strict_pool{loc};

    EXPECT_CALL(strict_trail, log(_)).Times(2);
    strict_pool.make(0);
    strict_pool.make(1);
}

// ---------------------------------------------------------------------------
// import
// ---------------------------------------------------------------------------

TEST_F(ExprPoolTest, ImportVarInternsIntoPool) {
    StrictMock<MockTrail> strict_trail;
    locator loc;
    loc.bind_as<i_log_to_current_trail_frame>(strict_trail);
    expr_pool strict_pool{loc};

    EXPECT_CALL(strict_trail, log(_)).Times(1);
    const expr* pooled = strict_pool.make(0);

    expr e{expr::var{0}};
    EXPECT_EQ(strict_pool.import(&e), pooled);
    EXPECT_EQ(strict_pool.import(&e), pooled);
}

TEST_F(ExprPoolTest, TernaryFunctorInternedTwiceReturnsSamePointer) {
    const expr* v0 = pool.make(0);
    const expr* v1 = pool.make(1);
    const expr* v2 = pool.make(2);
    EXPECT_EQ(pool.make("h", {v0, v1, v2}), pool.make("h", {v0, v1, v2}));
}

TEST_F(ExprPoolTest, ImportFunctorInternsArgsRecursively) {
    StrictMock<MockTrail> strict_trail;
    locator loc;
    loc.bind_as<i_log_to_current_trail_frame>(strict_trail);
    expr_pool strict_pool{loc};

    expr arg{expr::var{1}};
    expr root{expr::functor{"f", {&arg}}};

    EXPECT_CALL(strict_trail, log(_)).Times(2);
    const expr* p = strict_pool.import(&root);

    const expr::functor& f = std::get<expr::functor>(p->content);
    EXPECT_NE(f.args[0], &arg);
    EXPECT_EQ(f.args[0], strict_pool.make(1));
}

TEST_F(ExprPoolTest, ImportTernaryFunctorInternsEachArg) {
    StrictMock<MockTrail> strict_trail;
    locator loc;
    loc.bind_as<i_log_to_current_trail_frame>(strict_trail);
    expr_pool strict_pool{loc};

    expr a{expr::var{0}};
    expr b{expr::var{1}};
    expr c{expr::var{2}};
    expr root{expr::functor{"h", {&a, &b, &c}}};

    EXPECT_CALL(strict_trail, log(_)).Times(4);
    const expr* p = strict_pool.import(&root);

    const expr::functor& f = std::get<expr::functor>(p->content);
    ASSERT_EQ(f.args.size(), 3u);
    EXPECT_EQ(f.args[0], strict_pool.make(0));
    EXPECT_EQ(f.args[1], strict_pool.make(1));
    EXPECT_EQ(f.args[2], strict_pool.make(2));
}
