#include <sstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/expr_ostream_inserter.hpp"
#include "value_objects/expr.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::StrictMock;

namespace {

struct MockInsertUint8 {
    MOCK_METHOD(void, insert_uint8, (std::ostream&, uint8_t), (const));
};

struct MockInsertUint32 {
    MOCK_METHOD(void, insert_uint32, (std::ostream&, uint32_t), (const));
};

using sut_t = expr_ostream_inserter<MockInsertUint8, MockInsertUint32>;

struct ExprOstreamInserterTest : public ::testing::Test {
    StrictMock<MockInsertUint8> insert_uint8;
    StrictMock<MockInsertUint32> insert_uint32;
    sut_t sut{insert_uint8, insert_uint32};
    std::ostringstream out;
};

}

TEST_F(ExprOstreamInserterTest, VarWritesTagZeroAndIndex) {
    expr variable{expr::var{11}};
    InSequence seq;
    EXPECT_CALL(insert_uint8, insert_uint8(_, 0));
    EXPECT_CALL(insert_uint32, insert_uint32(_, 11u));
    sut.insert_expr(out, &variable);
}

TEST_F(ExprOstreamInserterTest, NullaryFunctorWritesTagArityAndId) {
    expr functor{expr::functor{4, {}}};
    InSequence seq;
    EXPECT_CALL(insert_uint8, insert_uint8(_, 1));
    EXPECT_CALL(insert_uint32, insert_uint32(_, 4u));
    EXPECT_CALL(insert_uint32, insert_uint32(_, 0u));
    sut.insert_expr(out, &functor);
}

TEST_F(ExprOstreamInserterTest, BinaryFunctorWritesArgsInOrder) {
    expr left{expr::functor{1, {}}};
    expr right{expr::functor{2, {}}};
    expr functor{expr::functor{9, {&left, &right}}};
    InSequence seq;
    EXPECT_CALL(insert_uint8, insert_uint8(_, 1));
    EXPECT_CALL(insert_uint32, insert_uint32(_, 9u));
    EXPECT_CALL(insert_uint32, insert_uint32(_, 2u));
    EXPECT_CALL(insert_uint8, insert_uint8(_, 1));
    EXPECT_CALL(insert_uint32, insert_uint32(_, 1u));
    EXPECT_CALL(insert_uint32, insert_uint32(_, 0u));
    EXPECT_CALL(insert_uint8, insert_uint8(_, 1));
    EXPECT_CALL(insert_uint32, insert_uint32(_, 2u));
    EXPECT_CALL(insert_uint32, insert_uint32(_, 0u));
    sut.insert_expr(out, &functor);
}
