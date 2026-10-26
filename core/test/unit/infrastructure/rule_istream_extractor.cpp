#include <sstream>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/rule_istream_extractor.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockExtractExpr {
    MOCK_METHOD(const expr*, extract_expr, (std::istream&), (const));
};

struct MockExtractUint64 {
    MOCK_METHOD(uint64_t, extract_uint64, (std::istream&), (const));
};

struct MockExtractUint32 {
    MOCK_METHOD(uint32_t, extract_uint32, (std::istream&), (const));
};

using sut_t = rule_istream_extractor<MockExtractExpr, MockExtractUint64, MockExtractUint32>;

struct RuleIstreamExtractorTest : public ::testing::Test {
    StrictMock<MockExtractExpr> extract_expr;
    StrictMock<MockExtractUint64> extract_uint64;
    StrictMock<MockExtractUint32> extract_uint32;
    sut_t sut{extract_expr, extract_uint64, extract_uint32};
    expr head{expr::functor{1, {}}};
    expr goal{expr::functor{2, {}}};
    std::istringstream in;
};

}

TEST_F(RuleIstreamExtractorTest, ReadsHeadBodyGoalsAndVarCount) {
    InSequence seq;
    EXPECT_CALL(extract_expr, extract_expr).WillOnce(Return(&head));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(1u));
    EXPECT_CALL(extract_expr, extract_expr).WillOnce(Return(&goal));
    EXPECT_CALL(extract_uint32, extract_uint32).WillOnce(Return(5u));
    const rule value = sut.extract_rule(in);
    EXPECT_EQ(value.head, &head);
    EXPECT_EQ(value.body.size(), 1u);
    EXPECT_EQ(value.body[0], &goal);
    EXPECT_EQ(value.var_count, 5u);
}
