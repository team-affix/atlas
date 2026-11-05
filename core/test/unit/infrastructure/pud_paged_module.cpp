#include <cstdint>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_paged_module.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_node_record.hpp"
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockRecords {
    MOCK_METHOD(size_t, count, (), (const));
    MOCK_METHOD(const pud_node_record&, get, (size_t), (const));
    MOCK_METHOD(void, set, (size_t, const pud_node_record&));
    MOCK_METHOD(void, append, (const pud_node_record&));
};
struct MockPayload {
    MOCK_METHOD(uint64_t, append, (const std::string&));
};
struct MockNameLog {
    MOCK_METHOD(void, append, (const std::string&));
};
struct MockNodeCache {
    MOCK_METHOD(const pud_inference&, get_inference, (pud_node_id));
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_node_id));
    MOCK_METHOD(void, keep_inference, (pud_node_id, pud_inference));
    MOCK_METHOD(void, forget_children, (pud_node_id));
    MOCK_METHOD(void, trim, ());
};
struct MockInsertInference {
    MOCK_METHOD(void, insert_inference, (std::ostream&, const pud_inference&), (const));
};

void pud_encode_as_abc(std::ostream& out, const pud_inference&) {
    out << "abc";
}

using test_module_t = pud_paged_module<
    NiceMock<MockRecords>, NiceMock<MockPayload>, NiceMock<MockNameLog>, NiceMock<MockNodeCache>,
    NiceMock<MockInsertInference>>;

pud_node_record record(uint64_t parent, uint64_t first_child, uint64_t child_count, uint64_t refuted) {
    return pud_node_record{
        .payload_offset = 0,
        .payload_size = 0,
        .parent = parent,
        .first_child = first_child,
        .child_count = child_count,
        .refuted = refuted};
}

// Module 3 has axioms 0 and 1; node 2 is axiom 0's only child so far; the files hold three records.
struct PudPagedModuleTest : public ::testing::Test {
    NiceMock<MockRecords> records;
    NiceMock<MockPayload> payload;
    NiceMock<MockNameLog> names;
    NiceMock<MockNodeCache> cache;
    NiceMock<MockInsertInference> inserter;
    expr head{.content = expr::var{.index = 0}};
    test_module_t module{
        records, payload, names, cache, inserter, 3, {1}, {rule(&head, {}, 0), rule(&head, {}, 1)}, {"zero"}};
    std::unordered_map<size_t, pud_node_record> stored{
        {0, record(pud_node_record::k_no_parent, 2, 1, 0)},
        {1, record(pud_node_record::k_no_parent, 0, 0, 1)},
        {2, record(0, 0, 0, 0)}};
    pud_inference inference{.parent = 0, .call_site = 4, .substitutions = {}, .body_goals = {}, .var_count = 2};

    void SetUp() override {
        ON_CALL(records, count()).WillByDefault(Return(3u));
        for (const auto& [node, kept] : stored)
            ON_CALL(records, get(node)).WillByDefault(ReturnRef(stored.at(node)));
        ON_CALL(inserter, insert_inference(_, _)).WillByDefault(Invoke(&pud_encode_as_abc));
        ON_CALL(payload, append(_)).WillByDefault(Return(100u));
    }
};

}

TEST_F(PudPagedModuleTest, TheHeaderIsHeldInMemory) {
    EXPECT_EQ(module.get_module_id(), 3u);
    EXPECT_THAT(module.get_dependencies(), ElementsAre(1u));
    EXPECT_EQ(module.count_axioms(), 2u);
    EXPECT_EQ(module.get_axiom(1).var_count, 1u);
    EXPECT_EQ(module.count_nodes(), 3u);
}

TEST_F(PudPagedModuleTest, ParentsLeavesAndRefutationComeFromTheRecords) {
    EXPECT_FALSE(module.get_parent(0).has_value());
    EXPECT_EQ(module.get_parent(2), 0u);
    EXPECT_FALSE(module.is_leaf(0));
    EXPECT_TRUE(module.is_leaf(2));
    EXPECT_TRUE(module.is_refuted(1));
    EXPECT_FALSE(module.is_refuted(2));
}

TEST_F(PudPagedModuleTest, InferencesAndChildrenAreReadThroughTheCache) {
    const std::vector<pud_node_id> children{2};
    EXPECT_CALL(cache, get_inference(2)).WillOnce(ReturnRef(inference));
    EXPECT_CALL(cache, get_children(0)).WillOnce(ReturnRef(children));
    EXPECT_EQ(&module.get_inference(2), &inference);
    EXPECT_EQ(&module.get_children(0), &children);
}

TEST_F(PudPagedModuleTest, AStoredInferenceIsEncodedAppendedAndKeptDecoded) {
    InSequence in_order;
    EXPECT_CALL(payload, append(std::string("abc"))).WillOnce(Return(100u));
    EXPECT_CALL(
        records,
        append(pud_node_record{
            .payload_offset = 100, .payload_size = 3, .parent = 0, .first_child = 0, .child_count = 0, .refuted = 0}));
    EXPECT_CALL(records, set(0u, record(pud_node_record::k_no_parent, 2, 2, 0)));
    EXPECT_CALL(cache, forget_children(0u));
    EXPECT_CALL(cache, keep_inference(3u, inference));
    EXPECT_EQ(module.store_inference(inference), 3u);
}

TEST_F(PudPagedModuleTest, AFirstChildStartsItsParentsRange) {
    inference.parent = 1;
    EXPECT_CALL(records, set(1u, record(pud_node_record::k_no_parent, 3, 1, 1)));
    module.store_inference(inference);
}

TEST_F(PudPagedModuleTest, MarkingRefutedSetsTheRecordsFlag) {
    EXPECT_CALL(records, set(2u, record(0, 0, 0, 1)));
    module.mark_refuted(2);
}

TEST_F(PudPagedModuleTest, ANewFunctorNameIsLoggedAndNumberedAfterTheOthers) {
    EXPECT_CALL(names, append(std::string("succ")));
    EXPECT_EQ(module.append_functor_name("succ"), 1u);
    EXPECT_THAT(module.functor_names(), ElementsAre("zero", "succ"));
}

TEST_F(PudPagedModuleTest, TrimmingTrimsTheCache) {
    EXPECT_CALL(cache, trim());
    module.trim();
}
