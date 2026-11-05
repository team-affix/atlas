#include <cstdint>
#include <istream>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_decoded_node_cache.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_node_record.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetRecord {
    MOCK_METHOD(const pud_node_record&, get, (size_t), (const));
};
struct MockReadPayload {
    MOCK_METHOD(std::string, read, (uint64_t, uint64_t), (const));
};
struct MockExtractInference {
    MOCK_METHOD(pud_inference, extract_inference, (std::istream&), (const));
};

// The decoded inference's var count is the payload byte it was decoded from.
pud_inference pud_decode_var_count(std::istream& in) {
    return pud_inference{
        .parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = static_cast<uint32_t>(in.get())};
}

using test_cache_t = pud_decoded_node_cache<
    NiceMock<MockGetRecord>, NiceMock<MockReadPayload>, NiceMock<MockExtractInference>>;

// Node 5 is an inference whose payload is the single byte 7, with children 9, 10 and 11; node 6 is a leaf.
struct PudDecodedNodeCacheTest : public ::testing::Test {
    NiceMock<MockGetRecord> records;
    NiceMock<MockReadPayload> payload;
    NiceMock<MockExtractInference> extractor;
    pud_node_record five{
        .payload_offset = 40, .payload_size = 1, .parent = 0, .first_child = 9, .child_count = 3, .refuted = 0};
    pud_node_record six{
        .payload_offset = 41, .payload_size = 1, .parent = 0, .first_child = 0, .child_count = 0, .refuted = 0};

    test_cache_t make_cache(size_t budget) { return test_cache_t{records, payload, extractor, budget}; }

    void SetUp() override {
        ON_CALL(records, get(5)).WillByDefault(ReturnRef(five));
        ON_CALL(records, get(6)).WillByDefault(ReturnRef(six));
        ON_CALL(payload, read(40u, 1u)).WillByDefault(Return(std::string(1, '\x07')));
        ON_CALL(payload, read(41u, 1u)).WillByDefault(Return(std::string(1, '\x08')));
        ON_CALL(extractor, extract_inference(_)).WillByDefault(Invoke(&pud_decode_var_count));
    }
};

}

TEST_F(PudDecodedNodeCacheTest, AnInferenceIsDecodedFromItsPayloadBytes) {
    test_cache_t cache = make_cache(1 << 20);
    EXPECT_CALL(payload, read(40u, 1u)).WillOnce(Return(std::string(1, '\x07')));
    EXPECT_EQ(cache.get_inference(5).var_count, 7u);
}

TEST_F(PudDecodedNodeCacheTest, AKeptInferenceIsNotDecodedAgain) {
    test_cache_t cache = make_cache(1 << 20);
    cache.get_inference(5);
    EXPECT_CALL(payload, read(_, _)).Times(0);
    cache.get_inference(5);
    EXPECT_EQ(cache.hits(), 1u);
    EXPECT_EQ(cache.misses(), 1u);
}

TEST_F(PudDecodedNodeCacheTest, ChildrenAreSpelledOutFromTheRecordsRange) {
    test_cache_t cache = make_cache(1 << 20);
    EXPECT_THAT(cache.get_children(5), ElementsAre(9u, 10u, 11u));
    EXPECT_THAT(cache.get_children(6), IsEmpty());
}

TEST_F(PudDecodedNodeCacheTest, ForgottenChildrenAreSpelledOutAgainFromTheGrownRange) {
    test_cache_t cache = make_cache(1 << 20);
    cache.get_children(6);
    six.first_child = 12;
    six.child_count = 2;
    cache.forget_children(6);
    EXPECT_THAT(cache.get_children(6), ElementsAre(12u, 13u));
}

TEST_F(PudDecodedNodeCacheTest, AKeptInferenceIsReadWithoutItsPayload) {
    test_cache_t cache = make_cache(1 << 20);
    cache.keep_inference(
        6, pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 3});
    EXPECT_CALL(payload, read(_, _)).Times(0);
    EXPECT_EQ(cache.get_inference(6).var_count, 3u);
}

TEST_F(PudDecodedNodeCacheTest, EntriesStayUntilTrimmedThenTheLeastRecentlyUsedGoFirst) {
    test_cache_t cache = make_cache(0);
    const pud_inference& kept = cache.get_inference(5);
    cache.get_inference(6);
    EXPECT_EQ(kept.var_count, 7u);
    cache.trim();
    EXPECT_CALL(payload, read(40u, 1u)).WillOnce(Return(std::string(1, '\x07')));
    cache.get_inference(5);
}
