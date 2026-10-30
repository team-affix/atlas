#include <optional>
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
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SizeIs;
using ::testing::StrictMock;

namespace {

struct MockMakeVar {
    MOCK_METHOD(const expr*, make_var, (uint32_t));
};

struct MockMakeFunctor {
    MOCK_METHOD(const expr*, make_functor, (uint32_t, (const std::vector<const expr*>&)));
};

using mock_decode_wiring_t = pud_module_serde_decode_wiring<MockMakeVar, MockMakeFunctor>;
using pool_decode_wiring_t = pud_module_serde_decode_wiring<expr_pool, expr_pool>;

std::string empty_module_blob(pud_module_id id) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, id);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 0);
    return blob;
}

std::string single_fact_blob(pud_module_id id, uint32_t functor_id) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, id);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_nullary_functor(blob, functor_id);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u32(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u8(blob, 0);
    return blob;
}

std::string var_head_axiom_blob(uint32_t var_index) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_var_expr(blob, var_index);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u32(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u8(blob, 0);
    return blob;
}

std::string binary_functor_head_blob(uint32_t id, uint32_t left_id, uint32_t right_id) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u8(blob, 1);
    pud_module_serde_bytes::append_u32(blob, id);
    pud_module_serde_bytes::append_u32(blob, 2);
    pud_module_serde_bytes::append_nullary_functor(blob, left_id);
    pud_module_serde_bytes::append_nullary_functor(blob, right_id);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u32(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u8(blob, 0);
    return blob;
}

std::string nested_functor_head_blob(uint32_t outer_id, uint32_t inner_id) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u8(blob, 1);
    pud_module_serde_bytes::append_u32(blob, outer_id);
    pud_module_serde_bytes::append_u32(blob, 1);
    pud_module_serde_bytes::append_nullary_functor(blob, inner_id);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u32(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u8(blob, 0);
    return blob;
}

std::string axiom_and_var_body_blob(uint32_t functor_id, uint32_t var_index) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_nullary_functor(blob, functor_id);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_var_expr(blob, var_index);
    pud_module_serde_bytes::append_u32(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u8(blob, 0);
    return blob;
}

std::string axiom_plus_inference_blob(uint32_t functor_id, pud_node_id parent, uint8_t refuted_child) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, 3);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 2);
    pud_module_serde_bytes::append_nullary_functor(blob, functor_id);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u32(blob, 0);
    pud_module_serde_bytes::append_nullary_functor(blob, functor_id + 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u32(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u64(blob, parent);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u32(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 3);
    pud_module_serde_bytes::append_u8(blob, 0);
    pud_module_serde_bytes::append_u8(blob, 0);
    pud_module_serde_bytes::append_u8(blob, refuted_child);
    return blob;
}

pud_module deserialize_pool(const std::string& blob, expr_pool& pool) {
    std::istringstream in(blob);
    pool_decode_wiring_t wiring{pool, pool};
    return wiring.extract_module_.extract_module(in);
}

struct PudModuleIstreamHandlingIntegrationTest : public ::testing::Test {
    expr_pool pool;
    test_functors functors;
    expr dummy_var{expr::var{0}};
    expr dummy_functor{expr::functor{0, {}}};
};

}

TEST_F(PudModuleIstreamHandlingIntegrationTest, MinimalValidBlobFromHandBuilderDeserializesToEmptyAxiomsModule) {
    const pud_module module = deserialize_pool(empty_module_blob(9), pool);
    EXPECT_EQ(module.get_module_id(), 9u);
    EXPECT_EQ(module.count_axioms(), 0u);
    EXPECT_EQ(module.count_nodes(), 0u);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, HandBuiltSingleAxiomDeserializes) {
    const uint32_t id = functors.id("p");
    const pud_module module = deserialize_pool(single_fact_blob(4, id), pool);
    EXPECT_EQ(module.get_module_id(), 4u);
    ASSERT_EQ(module.count_axioms(), 1u);
    EXPECT_EQ(module.get_axiom(0), rule(pool.make_functor(id, {}), {}, 0));
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, HandBuiltAxiomPlusOneInferenceReplaysStoreOrder) {
    const pud_module module = deserialize_pool(axiom_plus_inference_blob(functors.id("h"), 0, 0), pool);
    ASSERT_EQ(module.count_nodes(), 3u);
    EXPECT_EQ(module.get_parent(2), std::optional<pud_node_id>{0});
    EXPECT_THAT(module.get_children(0), testing::ElementsAre(2u));
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, RefutedFlagsAppliedAfterReplay) {
    const pud_module module = deserialize_pool(axiom_plus_inference_blob(functors.id("h"), 0, 1), pool);
    EXPECT_FALSE(module.check_refuted(0));
    EXPECT_FALSE(module.check_refuted(1));
    EXPECT_TRUE(module.check_refuted(2));
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, ReadingVarExprCallsMakeVarOnce) {
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockMakeFunctor> make_functor;
    EXPECT_CALL(make_var, make_var(7)).WillOnce(Return(&dummy_var));
    EXPECT_CALL(make_functor, make_functor).Times(0);
    std::istringstream in(var_head_axiom_blob(7));
    mock_decode_wiring_t wiring{make_var, make_functor};
    wiring.extract_module_.extract_module(in);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, ReadingFunctorCallsMakeFunctorWithArgCount) {
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockMakeFunctor> make_functor;
    EXPECT_CALL(make_functor, make_functor(8, SizeIs(0))).WillOnce(Return(&dummy_functor));
    EXPECT_CALL(make_functor, make_functor(9, SizeIs(0))).WillOnce(Return(&dummy_functor));
    EXPECT_CALL(make_functor, make_functor(4, SizeIs(2))).WillOnce(Return(&dummy_functor));
    std::istringstream in(binary_functor_head_blob(4, 8, 9));
    mock_decode_wiring_t wiring{make_var, make_functor};
    wiring.extract_module_.extract_module(in);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, NestedExprCallsMakeFunctorInPostOrder) {
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockMakeFunctor> make_functor;
    InSequence sequence;
    EXPECT_CALL(make_functor, make_functor(1, SizeIs(0))).WillOnce(Return(&dummy_functor));
    EXPECT_CALL(make_functor, make_functor(2, SizeIs(1))).WillOnce(Return(&dummy_functor));
    std::istringstream in(nested_functor_head_blob(2, 1));
    mock_decode_wiring_t wiring{make_var, make_functor};
    wiring.extract_module_.extract_module(in);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, WrongMagicThrows) {
    StrictMock<MockMakeVar> make_var;
    StrictMock<MockMakeFunctor> make_functor;
    std::istringstream in(pud_module_serde_bytes::replace_magic(empty_module_blob(1), "XXXX"));
    mock_decode_wiring_t wiring{make_var, make_functor};
    EXPECT_THROW(wiring.extract_module_.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, TruncatedAfterMagicThrows) {
    std::istringstream in(std::string(k_pud_module_blob_magic, 4));
    pool_decode_wiring_t wiring{pool, pool};
    EXPECT_THROW(wiring.extract_module_.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, TruncatedAfterModuleIdThrows) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, 1);
    std::istringstream in(blob);
    pool_decode_wiring_t wiring{pool, pool};
    EXPECT_THROW(wiring.extract_module_.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, TruncatedMidDependencyListThrows) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 3);
    pud_module_serde_bytes::append_u64(blob, 5);
    std::istringstream in(blob);
    pool_decode_wiring_t wiring{pool, pool};
    EXPECT_THROW(wiring.extract_module_.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, TruncatedMidAxiomHeadThrows) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u8(blob, 1);
    std::istringstream in(blob);
    pool_decode_wiring_t wiring{pool, pool};
    EXPECT_THROW(wiring.extract_module_.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, TruncatedMidRuleBodyThrows) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_nullary_functor(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 2);
    pud_module_serde_bytes::append_nullary_functor(blob, 2);
    std::istringstream in(blob);
    pool_decode_wiring_t wiring{pool, pool};
    EXPECT_THROW(wiring.extract_module_.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, InvalidExprTagThrows) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u8(blob, 2);
    std::istringstream in(blob);
    pool_decode_wiring_t wiring{pool, pool};
    EXPECT_THROW(wiring.extract_module_.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, TruncatedMidFunctorArgsThrows) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u8(blob, 1);
    pud_module_serde_bytes::append_u32(blob, 4);
    pud_module_serde_bytes::append_u32(blob, 3);
    pud_module_serde_bytes::append_nullary_functor(blob, 1);
    pud_module_serde_bytes::append_nullary_functor(blob, 2);
    std::istringstream in(blob);
    pool_decode_wiring_t wiring{pool, pool};
    EXPECT_THROW(wiring.extract_module_.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, TruncatedMidInferenceThrows) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_nullary_functor(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u32(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    std::istringstream in(blob);
    pool_decode_wiring_t wiring{pool, pool};
    EXPECT_THROW(wiring.extract_module_.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, TruncatedMidSubstitutionThrows) {
    std::string blob;
    pud_module_serde_bytes::append_magic(blob);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_nullary_functor(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u32(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 0);
    pud_module_serde_bytes::append_u64(blob, 1);
    pud_module_serde_bytes::append_u32(blob, 0);
    std::istringstream in(blob);
    pool_decode_wiring_t wiring{pool, pool};
    EXPECT_THROW(wiring.extract_module_.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, RefutedSectionTooShortThrows) {
    std::string blob = single_fact_blob(1, 1);
    blob.pop_back();
    std::istringstream in(blob);
    pool_decode_wiring_t wiring{pool, pool};
    EXPECT_THROW(wiring.extract_module_.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, ExtraBytesAfterNameSectionAreIgnored) {
    std::string blob = empty_module_blob(1);
    pud_module_serde_bytes::append_u64(blob, 0);
    blob.push_back('x');
    blob.push_back('y');
    const pud_module module = deserialize_pool(blob, pool);
    EXPECT_EQ(module.get_module_id(), 1u);
    EXPECT_EQ(module.count_nodes(), 0u);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, ParentIndexOutOfRangeThrows) {
    EXPECT_THROW(deserialize_pool(axiom_plus_inference_blob(functors.id("h"), 9, 0), pool), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, InferenceCountMismatchWithRefutedNodeCountThrows) {
    std::string blob = empty_module_blob(1);
    blob.resize(blob.size() - 8);
    pud_module_serde_bytes::append_u64(blob, 5);
    std::istringstream in(blob);
    pool_decode_wiring_t wiring{pool, pool};
    EXPECT_THROW(wiring.extract_module_.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, CorruptedByteInAxiomSectionThrows) {
    pud_module original{1, {}, {rule(pool.make_functor(functors.id("p"), {}), {}, 0)}};
    pud_module_serde_wiring encode_wiring;
    std::ostringstream out;
    encode_wiring.insert_module_.insert_module(out, original);
    const std::string corrupted = pud_module_serde_bytes::flip_byte(out.str(), 28);
    std::istringstream in(corrupted);
    pool_decode_wiring_t decode_wiring{pool, pool};
    EXPECT_THROW(decode_wiring.extract_module_.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, CorruptedByteInRefutedSectionThrows) {
    pud_module original{1, {}, {rule(pool.make_functor(functors.id("p"), {}), {}, 0)}};
    pud_module_serde_wiring encode_wiring;
    std::ostringstream out;
    encode_wiring.insert_module_.insert_module(out, original);
    std::string blob = out.str();
    blob[blob.size() - 9] = 2;
    std::istringstream in(blob);
    pool_decode_wiring_t decode_wiring{pool, pool};
    EXPECT_THROW(decode_wiring.extract_module_.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamHandlingIntegrationTest, MakeVarAndMakeFunctorCanBeDifferentObjects) {
    NiceMock<MockMakeVar> make_var;
    NiceMock<MockMakeFunctor> make_functor;
    EXPECT_CALL(make_functor, make_functor(3, SizeIs(0))).WillOnce(Return(&dummy_functor));
    EXPECT_CALL(make_var, make_var(4)).WillOnce(Return(&dummy_var));
    std::istringstream in(axiom_and_var_body_blob(3, 4));
    mock_decode_wiring_t wiring{make_var, make_functor};
    wiring.extract_module_.extract_module(in);
}
