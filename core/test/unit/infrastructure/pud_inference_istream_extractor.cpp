#include <sstream>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_inference_istream_extractor.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_substitution.hpp"

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

using sut_t = pud_inference_istream_extractor<MockExtractExpr, MockExtractUint64, MockExtractUint32>;

struct PudInferenceIstreamExtractorTest : public ::testing::Test {
    StrictMock<MockExtractExpr> extract_expr;
    StrictMock<MockExtractUint64> extract_uint64;
    StrictMock<MockExtractUint32> extract_uint32;
    sut_t sut{extract_expr, extract_uint64, extract_uint32};
    expr value_expr{expr::functor{1, {}}};
    expr goal{expr::functor{2, {}}};
    std::istringstream in;
};

}

TEST_F(PudInferenceIstreamExtractorTest, ReadsSubstitutionAndBodyGoals) {
    InSequence seq;
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(4u));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(2u));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(1u));
    EXPECT_CALL(extract_uint32, extract_uint32).WillOnce(Return(8u));
    EXPECT_CALL(extract_expr, extract_expr).WillOnce(Return(&value_expr));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(1u));
    EXPECT_CALL(extract_expr, extract_expr).WillOnce(Return(&goal));
    EXPECT_CALL(extract_uint32, extract_uint32).WillOnce(Return(6u));
    const pud_inference value = sut.extract_inference(in);
    EXPECT_EQ(value.parent, 4u);
    EXPECT_EQ(value.call_site, 2u);
    EXPECT_EQ(value.substitutions.size(), 1u);
    EXPECT_EQ(value.substitutions[0].var_idx, 8u);
    EXPECT_EQ(value.substitutions[0].value, &value_expr);
    EXPECT_EQ(value.body_goals.size(), 1u);
    EXPECT_EQ(value.body_goals[0], &goal);
    EXPECT_EQ(value.var_count, 6u);
}
