#include <sstream>
#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/expr_istream_extractor.hpp"
#include "value_objects/expr.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t));
};

struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor, (uint32_t, (const std::vector<const expr*>&)));
};

struct MockExtractUint8 {
    MOCK_METHOD(uint8_t, extract_uint8, (std::istream&), (const));
};

struct MockExtractUint32 {
    MOCK_METHOD(uint32_t, extract_uint32, (std::istream&), (const));
};

using sut_t = expr_istream_extractor<MockMakeVar, MockMakeFunctor, MockExtractUint8, MockExtractUint32>;

struct ExprIstreamExtractorTest : public ::testing::Test {
    StrictMock<MockMakeVar> make_var;
    StrictMock<MockMakeFunctor> make_functor;
    StrictMock<MockExtractUint8> extract_uint8;
    StrictMock<MockExtractUint32> extract_uint32;
    sut_t sut{make_var, make_functor, extract_uint8, extract_uint32};
    expr var_term{expr::var{0}};
    expr functor_term{expr::functor{0, {}}};
    std::istringstream in;
};

}

TEST_F(ExprIstreamExtractorTest, VarTagCallsMakeVar) {
    EXPECT_CALL(extract_uint8, extract_uint8).WillOnce(Return(0));
    EXPECT_CALL(extract_uint32, extract_uint32).WillOnce(Return(6u));
    EXPECT_CALL(make_var, make_var(6)).WillOnce(Return(&var_term));
    EXPECT_EQ(sut.extract_expr(in), &var_term);
}

TEST_F(ExprIstreamExtractorTest, FunctorTagBuildsArgsThenCallsMakeFunctor) {
    InSequence seq;
    EXPECT_CALL(extract_uint8, extract_uint8).WillOnce(Return(1));
    EXPECT_CALL(extract_uint32, extract_uint32).WillOnce(Return(3u));
    EXPECT_CALL(extract_uint32, extract_uint32).WillOnce(Return(0u));
    EXPECT_CALL(make_functor, make_functor(3, std::vector<const expr*>{})).WillOnce(Return(&functor_term));
    EXPECT_EQ(sut.extract_expr(in), &functor_term);
}

TEST_F(ExprIstreamExtractorTest, InvalidTagThrows) {
    EXPECT_CALL(extract_uint8, extract_uint8).WillOnce(Return(2));
    EXPECT_THROW(sut.extract_expr(in), std::runtime_error);
}
