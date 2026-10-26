#include <sstream>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_module_istream_extractor.hpp"
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockExtractFixedBytes {
    MOCK_METHOD(void, extract_fixed_bytes, (std::istream&, char*, size_t), (const));
};

struct MockExtractUint64 {
    MOCK_METHOD(uint64_t, extract_uint64, (std::istream&), (const));
};

struct MockDeserializeVectorUint64 {
    MOCK_METHOD((std::vector<uint64_t>), deserialize_vector_uint64, (std::istream&), (const));
};

struct MockExtractRule {
    MOCK_METHOD(rule, extract_rule, (std::istream&), (const));
};

struct MockExtractInference {
    MOCK_METHOD(pud_inference, extract_inference, (std::istream&), (const));
};

struct MockExtractUint8 {
    MOCK_METHOD(uint8_t, extract_uint8, (std::istream&), (const));
};

using sut_t = pud_module_istream_extractor<
    MockExtractFixedBytes,
    MockExtractUint64,
    MockDeserializeVectorUint64,
    MockExtractRule,
    MockExtractInference,
    MockExtractUint8>;

struct PudModuleIstreamExtractorTest : public ::testing::Test {
    StrictMock<MockExtractFixedBytes> extract_fixed_bytes;
    StrictMock<MockExtractUint64> extract_uint64;
    StrictMock<MockDeserializeVectorUint64> deserialize_vector_uint64;
    StrictMock<MockExtractRule> extract_rule;
    StrictMock<MockExtractInference> extract_inference;
    StrictMock<MockExtractUint8> extract_uint8;
    sut_t sut{
        extract_fixed_bytes,
        extract_uint64,
        deserialize_vector_uint64,
        extract_rule,
        extract_inference,
        extract_uint8};
    std::istringstream in;
};

void write_magic_to_buffer(char* magic) {
    magic[0] = 'P';
    magic[1] = 'U';
    magic[2] = 'D';
    magic[3] = 'M';
}

}

TEST_F(PudModuleIstreamExtractorTest, ValidMagicBuildsEmptyModule) {
    InSequence seq;
    EXPECT_CALL(extract_fixed_bytes, extract_fixed_bytes(_, _, 4))
        .WillOnce([](std::istream&, char* dest, size_t) { write_magic_to_buffer(dest); });
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(7u));
    EXPECT_CALL(deserialize_vector_uint64, deserialize_vector_uint64).WillOnce(Return(std::vector<uint64_t>{}));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(0u));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(0u));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(0u));
    const pud_module module = sut.extract_module(in);
    EXPECT_EQ(module.get_module_id(), 7u);
    EXPECT_EQ(module.count_nodes(), 0u);
}

TEST_F(PudModuleIstreamExtractorTest, WrongMagicThrows) {
    EXPECT_CALL(extract_fixed_bytes, extract_fixed_bytes(_, _, 4))
        .WillOnce([](std::istream&, char* dest, size_t) {
            dest[0] = 'X';
            dest[1] = 'Y';
            dest[2] = 'Z';
            dest[3] = 'W';
        });
    EXPECT_THROW(sut.extract_module(in), std::runtime_error);
}

TEST_F(PudModuleIstreamExtractorTest, AxiomsAreReadBeforeInferences) {
    expr head{expr::functor{1, {}}};
    const rule axiom{&head, {}, 0};
    pud_inference stored{
        .parent = 0,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {},
        .var_count = 0,
    };
    InSequence seq;
    EXPECT_CALL(extract_fixed_bytes, extract_fixed_bytes(_, _, 4))
        .WillOnce([](std::istream&, char* dest, size_t) { write_magic_to_buffer(dest); });
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(1u));
    EXPECT_CALL(deserialize_vector_uint64, deserialize_vector_uint64).WillOnce(Return(std::vector<uint64_t>{}));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(1u));
    EXPECT_CALL(extract_rule, extract_rule).WillOnce(Return(axiom));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(1u));
    EXPECT_CALL(extract_inference, extract_inference).WillOnce(Return(stored));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(2u));
    EXPECT_CALL(extract_uint8, extract_uint8).WillOnce(Return(0));
    EXPECT_CALL(extract_uint8, extract_uint8).WillOnce(Return(0));
    const pud_module module = sut.extract_module(in);
    EXPECT_EQ(module.count_axioms(), 1u);
    EXPECT_EQ(module.count_nodes(), 2u);
}

TEST_F(PudModuleIstreamExtractorTest, RefutedNodeCountMismatchThrows) {
    InSequence seq;
    EXPECT_CALL(extract_fixed_bytes, extract_fixed_bytes(_, _, 4))
        .WillOnce([](std::istream&, char* dest, size_t) { write_magic_to_buffer(dest); });
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(1u));
    EXPECT_CALL(deserialize_vector_uint64, deserialize_vector_uint64).WillOnce(Return(std::vector<uint64_t>{}));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(0u));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(0u));
    EXPECT_CALL(extract_uint64, extract_uint64).WillOnce(Return(3u));
    EXPECT_THROW(sut.extract_module(in), std::runtime_error);
}
