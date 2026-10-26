#include <sstream>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/expr_istream_extractor.hpp"
#include "infrastructure/expr_ostream_inserter.hpp"
#include "infrastructure/istream_fixed_bytes_extractor.hpp"
#include "infrastructure/istream_uint8_extractor.hpp"
#include "infrastructure/istream_uint32_extractor.hpp"
#include "infrastructure/istream_uint64_extractor.hpp"
#include "infrastructure/ostream_fixed_bytes_inserter.hpp"
#include "infrastructure/ostream_uint8_inserter.hpp"
#include "infrastructure/ostream_uint32_inserter.hpp"
#include "infrastructure/ostream_uint64_inserter.hpp"
#include "infrastructure/pud_inference_istream_extractor.hpp"
#include "infrastructure/pud_inference_ostream_inserter.hpp"
#include "infrastructure/rule_istream_extractor.hpp"
#include "infrastructure/rule_ostream_inserter.hpp"
#include "infrastructure/vector_uint64_istream_deserializer.hpp"
#include "infrastructure/vector_uint64_ostream_serializer.hpp"
#include "infrastructure/expr_pool.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_substitution.hpp"
#include "value_objects/rule.hpp"

namespace {

void expect_same_expr(const expr* left, const expr* right) {
    if (const expr::var* left_var = std::get_if<expr::var>(&left->content)) {
        const expr::var* right_var = std::get_if<expr::var>(&right->content);
        ASSERT_NE(right_var, nullptr);
        EXPECT_EQ(left_var->index, right_var->index);
        return;
    }
    const expr::functor* left_functor = std::get_if<expr::functor>(&left->content);
    const expr::functor* right_functor = std::get_if<expr::functor>(&right->content);
    ASSERT_NE(left_functor, nullptr);
    ASSERT_NE(right_functor, nullptr);
    EXPECT_EQ(left_functor->id, right_functor->id);
    EXPECT_EQ(left_functor->args.size(), right_functor->args.size());
    for (size_t arg = 0; arg < left_functor->args.size(); ++arg)
        expect_same_expr(left_functor->args[arg], right_functor->args[arg]);
}

struct PudStreamCodecLayersIntegrationTest : public ::testing::Test {
    test_functors functors;
    expr_pool encode_pool;
    expr_pool decode_pool;

    ostream_uint8_inserter insert_uint8;
    ostream_uint32_inserter insert_uint32;
    ostream_uint64_inserter insert_uint64;
    ostream_fixed_bytes_inserter insert_fixed_bytes;
    istream_uint8_extractor extract_uint8;
    istream_uint32_extractor extract_uint32;
    istream_uint64_extractor extract_uint64;
    istream_fixed_bytes_extractor extract_fixed_bytes;

    expr_ostream_inserter<ostream_uint8_inserter, ostream_uint32_inserter> insert_expr{
        insert_uint8,
        insert_uint32};
    expr_istream_extractor<expr_pool, expr_pool, istream_uint8_extractor, istream_uint32_extractor> extract_expr{
        decode_pool,
        decode_pool,
        extract_uint8,
        extract_uint32};

    rule_ostream_inserter<
        expr_ostream_inserter<ostream_uint8_inserter, ostream_uint32_inserter>,
        ostream_uint64_inserter,
        ostream_uint32_inserter> insert_rule{insert_expr, insert_uint64, insert_uint32};
    rule_istream_extractor<
        expr_istream_extractor<expr_pool, expr_pool, istream_uint8_extractor, istream_uint32_extractor>,
        istream_uint64_extractor,
        istream_uint32_extractor> extract_rule{extract_expr, extract_uint64, extract_uint32};

    pud_inference_ostream_inserter<
        expr_ostream_inserter<ostream_uint8_inserter, ostream_uint32_inserter>,
        ostream_uint64_inserter,
        ostream_uint32_inserter> insert_inference{insert_expr, insert_uint64, insert_uint32};
    pud_inference_istream_extractor<
        expr_istream_extractor<expr_pool, expr_pool, istream_uint8_extractor, istream_uint32_extractor>,
        istream_uint64_extractor,
        istream_uint32_extractor> extract_inference{extract_expr, extract_uint64, extract_uint32};

    vector_uint64_ostream_serializer<ostream_uint64_inserter> serialize_vector_uint64{insert_uint64};
    vector_uint64_istream_deserializer<istream_uint64_extractor> deserialize_vector_uint64{extract_uint64};
};

}

TEST_F(PudStreamCodecLayersIntegrationTest, UintPrimitivesRoundTrip) {
    std::ostringstream out;
    insert_uint8.insert_uint8(out, 0xabu);
    insert_uint32.insert_uint32(out, 0x04030201u);
    insert_uint64.insert_uint64(out, 0x0807060504030201ull);
    insert_fixed_bytes.insert_fixed_bytes(out, "PUD", 3);
    std::istringstream in(out.str());
    EXPECT_EQ(extract_uint8.extract_uint8(in), 0xabu);
    EXPECT_EQ(extract_uint32.extract_uint32(in), 0x04030201u);
    EXPECT_EQ(extract_uint64.extract_uint64(in), 0x0807060504030201ull);
    char magic[3] = {};
    extract_fixed_bytes.extract_fixed_bytes(in, magic, 3);
    EXPECT_EQ(magic[0], 'P');
    EXPECT_EQ(magic[2], 'D');
}

TEST_F(PudStreamCodecLayersIntegrationTest, VectorUint64RoundTrip) {
    std::ostringstream out;
    const std::vector<uint64_t> items{1, 99, 0x123456789abcdef0ull};
    serialize_vector_uint64.serialize_vector_uint64(out, items);
    std::istringstream in(out.str());
    EXPECT_EQ(deserialize_vector_uint64.deserialize_vector_uint64(in), items);
}

TEST_F(PudStreamCodecLayersIntegrationTest, ExprRoundTripUsesDistinctPools) {
    const expr* original = encode_pool.make_functor(
        functors.id("f"),
        {encode_pool.make_var(2), encode_pool.make_functor(functors.id("g"), {})});
    std::ostringstream out;
    insert_expr.insert_expr(out, original);
    std::istringstream in(out.str());
    const expr* loaded = extract_expr.extract_expr(in);
    expect_same_expr(original, loaded);
    EXPECT_NE(original, loaded);
}

TEST_F(PudStreamCodecLayersIntegrationTest, RuleRoundTrip) {
    const expr* head = encode_pool.make_functor(functors.id("h"), {});
    const expr* goal = encode_pool.make_var(4);
    const rule original{head, {goal}, 5};
    std::ostringstream out;
    insert_rule.insert_rule(out, original);
    std::istringstream in(out.str());
    const rule loaded = extract_rule.extract_rule(in);
    expect_same_expr(original.head, loaded.head);
    EXPECT_EQ(loaded.body.size(), 1u);
    expect_same_expr(original.body[0], loaded.body[0]);
    EXPECT_EQ(loaded.var_count, 5u);
}

TEST_F(PudStreamCodecLayersIntegrationTest, InferenceRoundTrip) {
    const expr* substitution_value = encode_pool.make_functor(functors.id("v"), {});
    const expr* goal = encode_pool.make_var(1);
    const pud_inference original{
        .parent = 3,
        .call_site = 2,
        .substitutions = {pud_substitution{.var_idx = 6, .value = substitution_value}},
        .body_goals = {goal},
        .var_count = 4,
    };
    std::ostringstream out;
    insert_inference.insert_inference(out, original);
    std::istringstream in(out.str());
    const pud_inference loaded = extract_inference.extract_inference(in);
    EXPECT_EQ(loaded.parent, original.parent);
    EXPECT_EQ(loaded.call_site, original.call_site);
    EXPECT_EQ(loaded.substitutions.size(), 1u);
    EXPECT_EQ(loaded.substitutions[0].var_idx, 6u);
    expect_same_expr(loaded.substitutions[0].value, substitution_value);
    EXPECT_EQ(loaded.body_goals.size(), 1u);
    expect_same_expr(loaded.body_goals[0], goal);
    EXPECT_EQ(loaded.var_count, 4u);
}
