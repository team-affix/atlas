#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "pud_module_serde_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/rule.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace {

struct parsed_expr {
    bool is_var;
    uint32_t index_or_id;
    std::vector<parsed_expr> args;
};

parsed_expr parse_expr(pud_module_serde_cursor& cursor) {
    const uint8_t tag = cursor.read_u8();
    if (tag == 0)
        return parsed_expr{.is_var = true, .index_or_id = cursor.read_u32(), .args = {}};
    EXPECT_EQ(tag, 1u);
    parsed_expr functor{.is_var = false, .index_or_id = cursor.read_u32(), .args = {}};
    const uint32_t argc = cursor.read_u32();
    functor.args.reserve(argc);
    for (uint32_t arg = 0; arg < argc; ++arg)
        functor.args.push_back(parse_expr(cursor));
    return functor;
}

struct parsed_rule {
    parsed_expr head;
    std::vector<parsed_expr> body;
    uint32_t var_count;
};

parsed_rule parse_rule(pud_module_serde_cursor& cursor) {
    parsed_rule axiom{.head = parse_expr(cursor), .body = {}, .var_count = 0};
    const uint64_t body_len = cursor.read_u64();
    axiom.body.reserve(body_len);
    for (uint64_t goal = 0; goal < body_len; ++goal)
        axiom.body.push_back(parse_expr(cursor));
    axiom.var_count = cursor.read_u32();
    return axiom;
}

struct parsed_inference {
    uint64_t parent;
    uint64_t call_site;
    std::vector<std::pair<uint32_t, parsed_expr>> substitutions;
    std::vector<parsed_expr> body_goals;
    uint32_t var_count;
};

parsed_inference parse_inference(pud_module_serde_cursor& cursor) {
    parsed_inference inference{
        .parent = cursor.read_u64(),
        .call_site = cursor.read_u64(),
        .substitutions = {},
        .body_goals = {},
        .var_count = 0,
    };
    const uint64_t substitution_count = cursor.read_u64();
    inference.substitutions.reserve(substitution_count);
    for (uint64_t substitution = 0; substitution < substitution_count; ++substitution) {
        const uint32_t var_idx = cursor.read_u32();
        inference.substitutions.push_back({var_idx, parse_expr(cursor)});
    }
    const uint64_t body_len = cursor.read_u64();
    inference.body_goals.reserve(body_len);
    for (uint64_t goal = 0; goal < body_len; ++goal)
        inference.body_goals.push_back(parse_expr(cursor));
    inference.var_count = cursor.read_u32();
    return inference;
}

std::string insert_module_blob(pud_module_serde_wiring& wiring, const pud_module& module) {
    std::ostringstream out;
    wiring.insert_module_.insert_module(out, module);
    return out.str();
}

struct PudModuleOstreamLayoutIntegrationTest : public ::testing::Test {
    pud_module_serde_wiring wiring;
    test_functors functors;
    expr_pool pool;
    expr stack_head{expr::functor{1, {}}};
    expr stack_goal{expr::functor{2, {}}};
    expr stack_value{expr::functor{3, {}}};
};

}

TEST_F(PudModuleOstreamLayoutIntegrationTest, SerializedBlobStartsWithMagic) {
    const std::string blob = insert_module_blob(wiring, make_empty_pud_module(1));
    ASSERT_GE(blob.size(), 4u);
    EXPECT_EQ(blob[0], 'P');
    EXPECT_EQ(blob[1], 'U');
    EXPECT_EQ(blob[2], 'D');
    EXPECT_EQ(blob[3], 'M');
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, ModuleIdAndDependencyCountAppearAfterHeader) {
    pud_module module{42, {5, 9}, {}};
    const std::string blob = insert_module_blob(wiring, module);
    EXPECT_EQ(pud_module_serde_bytes::peek_u64(blob, 4), 42u);
    EXPECT_EQ(pud_module_serde_bytes::peek_u64(blob, 12), 2u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, DependenciesAreWrittenInOrder) {
    pud_module module{7, {5, 9, 1}, {}};
    const std::string blob = insert_module_blob(wiring, module);
    EXPECT_EQ(pud_module_serde_bytes::peek_u64(blob, 20), 5u);
    EXPECT_EQ(pud_module_serde_bytes::peek_u64(blob, 28), 9u);
    EXPECT_EQ(pud_module_serde_bytes::peek_u64(blob, 36), 1u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, ZeroDependenciesWritesCountZero) {
    const std::string blob = insert_module_blob(wiring, make_empty_pud_module(3));
    EXPECT_EQ(pud_module_serde_bytes::peek_u64(blob, 12), 0u);
    EXPECT_EQ(pud_module_serde_bytes::peek_u64(blob, 20), 0u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, LargeModuleIdRoundTripsThroughBytes) {
    pud_module module{std::numeric_limits<uint64_t>::max(), {}, {}};
    const std::string blob = insert_module_blob(wiring, module);
    EXPECT_EQ(pud_module_serde_bytes::peek_u64(blob, 4), std::numeric_limits<uint64_t>::max());
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, ZeroAxiomsWritesCountZero) {
    const std::string blob = insert_module_blob(wiring, make_empty_pud_module(1));
    EXPECT_EQ(pud_module_serde_bytes::peek_u64(blob, 20), 0u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, SingleFactAxiomWritesCountOne) {
    pud_module module{1, {}, {rule(&stack_head, {}, 0)}};
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    EXPECT_EQ(cursor.read_u64(), 1u);
    const parsed_rule axiom = parse_rule(cursor);
    EXPECT_FALSE(axiom.head.is_var);
    EXPECT_EQ(axiom.head.index_or_id, 1u);
    EXPECT_THAT(axiom.head.args, IsEmpty());
    EXPECT_THAT(axiom.body, IsEmpty());
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, MultipleAxiomsWrittenInIndexOrder) {
    pud_module module{1, {}, {rule(&stack_head, {}, 0), rule(&stack_goal, {}, 0)}};
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    EXPECT_EQ(cursor.read_u64(), 2u);
    EXPECT_EQ(parse_rule(cursor).head.index_or_id, 1u);
    EXPECT_EQ(parse_rule(cursor).head.index_or_id, 2u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, RuleBodyLengthMatchesGoalCount) {
    expr first{expr::functor{10, {}}};
    expr second{expr::functor{11, {}}};
    expr third{expr::functor{12, {}}};
    pud_module module{
        1,
        {},
        {
            rule(&stack_head, {}, 0),
            rule(&stack_head, {&first}, 0),
            rule(&stack_head, {&first, &second, &third}, 0),
        },
    };
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    EXPECT_EQ(cursor.read_u64(), 3u);
    EXPECT_EQ(parse_rule(cursor).body.size(), 0u);
    EXPECT_EQ(parse_rule(cursor).body.size(), 1u);
    EXPECT_EQ(parse_rule(cursor).body.size(), 3u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, RuleVarCountIsWritten) {
    pud_module module{1, {}, {rule(&stack_head, {}, 0), rule(&stack_goal, {}, 4)}};
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    cursor.read_u64();
    EXPECT_EQ(parse_rule(cursor).var_count, 0u);
    EXPECT_EQ(parse_rule(cursor).var_count, 4u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, VarExprEncodesTagZeroAndIndex) {
    expr head{expr::var{7}};
    pud_module module{1, {}, {rule(&head, {}, 1)}};
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    cursor.read_u64();
    const parsed_expr term = parse_rule(cursor).head;
    EXPECT_TRUE(term.is_var);
    EXPECT_EQ(term.index_or_id, 7u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, FunctorExprEncodesTagOneIdAndArgc) {
    expr left{expr::functor{8, {}}};
    expr right{expr::functor{9, {}}};
    expr binary{expr::functor{4, {&left, &right}}};
    pud_module module{1, {}, {rule(&stack_head, {}, 0), rule(&binary, {}, 0)}};
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    cursor.read_u64();
    const parsed_expr nullary = parse_rule(cursor).head;
    EXPECT_FALSE(nullary.is_var);
    EXPECT_EQ(nullary.index_or_id, 1u);
    EXPECT_THAT(nullary.args, IsEmpty());
    const parsed_expr parsed_binary = parse_rule(cursor).head;
    EXPECT_FALSE(parsed_binary.is_var);
    EXPECT_EQ(parsed_binary.index_or_id, 4u);
    ASSERT_EQ(parsed_binary.args.size(), 2u);
    EXPECT_EQ(parsed_binary.args[0].index_or_id, 8u);
    EXPECT_EQ(parsed_binary.args[1].index_or_id, 9u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, NestedFunctorExprIsPrefixStructured) {
    expr inner{expr::functor{2, {}}};
    expr mid{expr::functor{3, {&inner}}};
    expr outer{expr::functor{4, {&mid}}};
    pud_module module{1, {}, {rule(&outer, {}, 0)}};
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    cursor.read_u64();
    const parsed_expr term = parse_rule(cursor).head;
    EXPECT_EQ(term.index_or_id, 4u);
    ASSERT_EQ(term.args.size(), 1u);
    EXPECT_EQ(term.args[0].index_or_id, 3u);
    ASSERT_EQ(term.args[0].args.size(), 1u);
    EXPECT_EQ(term.args[0].args[0].index_or_id, 2u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, SharedSubexprViaSamePointerWritesDuplicateTrees) {
    pud_module module{1, {}, {rule(&stack_head, {&stack_goal, &stack_goal}, 0)}};
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    cursor.read_u64();
    const parsed_rule axiom = parse_rule(cursor);
    ASSERT_EQ(axiom.body.size(), 2u);
    EXPECT_EQ(axiom.body[0].index_or_id, axiom.body[1].index_or_id);
    EXPECT_EQ(axiom.body[0].index_or_id, 2u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, SubstitutionValueExprEncodedInsideInference) {
    pud_module module{1, {}, {rule(&stack_head, {&stack_goal}, 0)}};
    module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = &stack_value}},
        .body_goals = {},
        .var_count = 0,
    });
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    cursor.read_u64();
    parse_rule(cursor);
    EXPECT_EQ(cursor.read_u64(), 1u);
    const parsed_inference inference = parse_inference(cursor);
    ASSERT_EQ(inference.substitutions.size(), 1u);
    EXPECT_EQ(inference.substitutions[0].first, 0u);
    EXPECT_EQ(inference.substitutions[0].second.index_or_id, 3u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, ZeroInferencesWritesInferenceCountZero) {
    pud_module module{1, {}, {rule(&stack_head, {}, 0)}};
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    cursor.read_u64();
    parse_rule(cursor);
    EXPECT_EQ(cursor.read_u64(), 0u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, InferenceCountMatchesStoredInferences) {
    pud_module module = make_chain_pud_module(1, pool, functors, 3);
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    cursor.read_u64();
    parse_rule(cursor);
    EXPECT_EQ(cursor.read_u64(), 3u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, InferencesSerializedInNodeIdOrder) {
    pud_module module = make_branchy_pud_module(1, pool, functors);
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    const uint64_t dependency_count = cursor.read_u64();
    for (uint64_t dependency = 0; dependency < dependency_count; ++dependency)
        cursor.read_u64();
    EXPECT_EQ(cursor.read_u64(), 2u);
    parse_rule(cursor);
    parse_rule(cursor);
    EXPECT_EQ(cursor.read_u64(), 4u);
    EXPECT_EQ(parse_inference(cursor).parent, 0u);
    EXPECT_EQ(parse_inference(cursor).parent, 0u);
    EXPECT_EQ(parse_inference(cursor).parent, 2u);
    EXPECT_EQ(parse_inference(cursor).parent, 1u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, CallSiteAndVarCountPresentPerInference) {
    pud_module module{1, {}, {rule(&stack_head, {&stack_goal}, 0)}};
    module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 3,
        .substitutions = {},
        .body_goals = {},
        .var_count = 5,
    });
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    cursor.read_u64();
    parse_rule(cursor);
    cursor.read_u64();
    const parsed_inference inference = parse_inference(cursor);
    EXPECT_EQ(inference.call_site, 3u);
    EXPECT_EQ(inference.var_count, 5u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, RefutedNodeCountEqualsTotalNodes) {
    pud_module module = make_chain_pud_module(1, pool, functors, 2);
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    cursor.read_u64();
    parse_rule(cursor);
    cursor.read_u64();
    parse_inference(cursor);
    parse_inference(cursor);
    EXPECT_EQ(cursor.read_u64(), 3u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, AllRefutedFlagsZeroWhenNothingRefuted) {
    pud_module module{1, {}, {rule(&stack_head, {}, 0), rule(&stack_goal, {}, 0)}};
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    cursor.read_u64();
    parse_rule(cursor);
    parse_rule(cursor);
    cursor.read_u64();
    EXPECT_EQ(cursor.read_u64(), 2u);
    EXPECT_EQ(cursor.read_u8(), 0u);
    EXPECT_EQ(cursor.read_u8(), 0u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, RefutedFlagOneAtKnownIndex) {
    pud_module module{1, {}, {rule(&stack_head, {}, 0), rule(&stack_goal, {}, 0)}};
    module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {},
        .var_count = 0,
    });
    module.mark_refuted(2);
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    cursor.read_u64();
    parse_rule(cursor);
    parse_rule(cursor);
    cursor.read_u64();
    parse_inference(cursor);
    EXPECT_EQ(cursor.read_u64(), 3u);
    EXPECT_EQ(cursor.read_u8(), 0u);
    EXPECT_EQ(cursor.read_u8(), 0u);
    EXPECT_EQ(cursor.read_u8(), 1u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, MultipleRefutedNodesSetMultipleFlags) {
    pud_module module{1, {}, {rule(&stack_head, {}, 0)}};
    module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {},
        .var_count = 0,
    });
    module.mark_refuted(0);
    module.mark_refuted(1);
    const std::string blob = insert_module_blob(wiring, module);
    pud_module_serde_cursor cursor{blob};
    cursor.expect_magic();
    cursor.read_u64();
    cursor.read_u64();
    cursor.read_u64();
    parse_rule(cursor);
    cursor.read_u64();
    parse_inference(cursor);
    cursor.read_u64();
    EXPECT_EQ(cursor.read_u8(), 1u);
    EXPECT_EQ(cursor.read_u8(), 1u);
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, SerializeAppendsToNonEmptyStream) {
    std::ostringstream out;
    out << "junk";
    wiring.insert_module_.insert_module(out, make_empty_pud_module(1));
    const std::string blob = out.str();
    ASSERT_GE(blob.size(), 8u);
    EXPECT_EQ(blob.substr(0, 4), "junk");
    EXPECT_EQ(blob.substr(4, 4), "PUDM");
}

TEST_F(PudModuleOstreamLayoutIntegrationTest, SerializeOnEmptyModuleProducesOnlyHeaderSections) {
    const std::string blob = insert_module_blob(wiring, make_empty_pud_module(1));
    EXPECT_EQ(blob.size(), 4u + 8u + 8u + 8u + 8u + 8u);
}
