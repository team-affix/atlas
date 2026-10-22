#include <cstdint>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_touched_var_logger.hpp"

using ::testing::InSequence;
using ::testing::StrictMock;

namespace {

struct MockEquate {
    MOCK_METHOD(void, equate, (uint32_t, uint32_t));
};
struct MockInstantiate {
    MOCK_METHOD(void, instantiate, (uint32_t, framed_expr));
};
struct MockPushBackTouchedVar {
    MOCK_METHOD(void, push_back, (uint32_t));
};

using test_logger_t = pud_touched_var_logger<
    StrictMock<MockEquate>, StrictMock<MockInstantiate>, StrictMock<MockPushBackTouchedVar>>;

struct PudTouchedVarLoggerTest : public ::testing::Test {
    StrictMock<MockEquate> equate;
    StrictMock<MockInstantiate> instantiate;
    StrictMock<MockPushBackTouchedVar> touched;
    test_logger_t logger{equate, instantiate, touched};
    expr value{expr::functor{3, {}}};
};

}

TEST_F(PudTouchedVarLoggerTest, EquateForwardsThenRecordsBothVars) {
    InSequence in_order;
    EXPECT_CALL(equate, equate(4u, 9u));
    EXPECT_CALL(touched, push_back(4u));
    EXPECT_CALL(touched, push_back(9u));
    logger.equate(4, 9);
}

TEST_F(PudTouchedVarLoggerTest, InstantiateForwardsThenRecordsTheVar) {
    InSequence in_order;
    EXPECT_CALL(instantiate, instantiate(7u, (framed_expr{&value, 2})));
    EXPECT_CALL(touched, push_back(7u));
    logger.instantiate(7, framed_expr{&value, 2});
}
