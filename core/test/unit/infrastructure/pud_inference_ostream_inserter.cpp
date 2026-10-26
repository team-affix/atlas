#include <sstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_inference_ostream_inserter.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_substitution.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::StrictMock;

namespace {

struct MockInsertExpr {
    MOCK_METHOD(void, insert_expr, (std::ostream&, const expr*), (const));
};

struct MockInsertUint64 {
    MOCK_METHOD(void, insert_uint64, (std::ostream&, uint64_t), (const));
};

struct MockInsertUint32 {
    MOCK_METHOD(void, insert_uint32, (std::ostream&, uint32_t), (const));
};

using sut_t = pud_inference_ostream_inserter<MockInsertExpr, MockInsertUint64, MockInsertUint32>;

struct PudInferenceOstreamInserterTest : public ::testing::Test {
    StrictMock<MockInsertExpr> insert_expr;
    StrictMock<MockInsertUint64> insert_uint64;
    StrictMock<MockInsertUint32> insert_uint32;
    sut_t sut{insert_expr, insert_uint64, insert_uint32};
    expr value_expr{expr::functor{1, {}}};
    expr goal{expr::functor{2, {}}};
    std::ostringstream out;
};

}

TEST_F(PudInferenceOstreamInserterTest, WritesHeaderSubstitutionAndBodySections) {
    const pud_inference value{
        .parent = 2,
        .call_site = 1,
        .substitutions = {pud_substitution{.var_idx = 7, .value = &value_expr}},
        .body_goals = {&goal},
        .var_count = 3,
    };
    InSequence seq;
    EXPECT_CALL(insert_uint64, insert_uint64(_, 2u));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 1u));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 1u));
    EXPECT_CALL(insert_uint32, insert_uint32(_, 7u));
    EXPECT_CALL(insert_expr, insert_expr(_, &value_expr));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 1u));
    EXPECT_CALL(insert_expr, insert_expr(_, &goal));
    EXPECT_CALL(insert_uint32, insert_uint32(_, 3u));
    sut.insert_inference(out, value);
}
