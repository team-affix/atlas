#include <sstream>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_module_ostream_inserter.hpp"
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::StrictMock;

namespace {

struct MockInsertFixedBytes {
    MOCK_METHOD(void, insert_fixed_bytes, (std::ostream&, const char*, size_t), (const));
};

struct MockInsertUint64 {
    MOCK_METHOD(void, insert_uint64, (std::ostream&, uint64_t), (const));
};

struct MockSerializeVectorUint64 {
    MOCK_METHOD(void, serialize_vector_uint64, (std::ostream&, (const std::vector<uint64_t>&)), (const));
};

struct MockInsertRule {
    MOCK_METHOD(void, insert_rule, (std::ostream&, const rule&), (const));
};

struct MockInsertInference {
    MOCK_METHOD(void, insert_inference, (std::ostream&, const pud_inference&), (const));
};

struct MockInsertUint8 {
    MOCK_METHOD(void, insert_uint8, (std::ostream&, uint8_t), (const));
};

using sut_t = pud_module_ostream_inserter<
    MockInsertFixedBytes,
    MockInsertUint64,
    MockSerializeVectorUint64,
    MockInsertRule,
    MockInsertInference,
    MockInsertUint8>;

struct PudModuleOstreamInserterTest : public ::testing::Test {
    StrictMock<MockInsertFixedBytes> insert_fixed_bytes;
    StrictMock<MockInsertUint64> insert_uint64;
    StrictMock<MockSerializeVectorUint64> serialize_vector_uint64;
    StrictMock<MockInsertRule> insert_rule;
    StrictMock<MockInsertInference> insert_inference;
    StrictMock<MockInsertUint8> insert_uint8;
    sut_t sut{
        insert_fixed_bytes,
        insert_uint64,
        serialize_vector_uint64,
        insert_rule,
        insert_inference,
        insert_uint8};
    std::ostringstream out;
};

}

TEST_F(PudModuleOstreamInserterTest, EmptyModuleInsertsMagicAndSectionCounts) {
    const pud_module module{9, {}, {}};
    InSequence seq;
    EXPECT_CALL(insert_fixed_bytes, insert_fixed_bytes(_, _, 4));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 9u));
    EXPECT_CALL(serialize_vector_uint64, serialize_vector_uint64(_, _));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 0u));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 0u));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 0u));
    sut.insert_module(out, module);
}

TEST_F(PudModuleOstreamInserterTest, EachAxiomIsInsertedInOrder) {
    expr head{expr::functor{1, {}}};
    const rule first{&head, {}, 0};
    const rule second{&head, {}, 1};
    pud_module module{1, {}, {first, second}};
    InSequence seq;
    EXPECT_CALL(insert_fixed_bytes, insert_fixed_bytes(_, _, 4));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 1u));
    EXPECT_CALL(serialize_vector_uint64, serialize_vector_uint64(_, _));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 2u));
    EXPECT_CALL(insert_rule, insert_rule(_, _));
    EXPECT_CALL(insert_rule, insert_rule(_, _));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 0u));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 2u));
    EXPECT_CALL(insert_uint8, insert_uint8(_, 0));
    EXPECT_CALL(insert_uint8, insert_uint8(_, 0));
    sut.insert_module(out, module);
}

TEST_F(PudModuleOstreamInserterTest, RefutedFlagsFollowNodeOrder) {
    expr head{expr::functor{1, {}}};
    pud_module module{1, {}, {rule(&head, {}, 0)}};
    module.store_inference(pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0});
    module.mark_refuted(1);
    InSequence seq;
    EXPECT_CALL(insert_fixed_bytes, insert_fixed_bytes(_, _, 4));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 1u));
    EXPECT_CALL(serialize_vector_uint64, serialize_vector_uint64(_, _));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 1u));
    EXPECT_CALL(insert_rule, insert_rule(_, _));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 1u));
    EXPECT_CALL(insert_inference, insert_inference(_, _));
    EXPECT_CALL(insert_uint64, insert_uint64(_, 2u));
    EXPECT_CALL(insert_uint8, insert_uint8(_, 0));
    EXPECT_CALL(insert_uint8, insert_uint8(_, 1));
    sut.insert_module(out, module);
}
