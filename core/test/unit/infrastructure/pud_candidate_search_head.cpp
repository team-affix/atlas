#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_candidate_search_head.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

using child_iter = std::vector<const pud_node*>::const_iterator;

struct MockTryAdd {
    MOCK_METHOD(std::optional<pud_mhws_head_id>, try_add_head, (pud_query_position<int>));
};

struct MockAdvance {
    MOCK_METHOD((pud_query_frame<int, child_iter>), advance_head, (pud_mhws_head_id));
};

struct MockForkWitness {
    MOCK_METHOD(std::optional<pud_mhws_head_id>, try_fork_head, (pud_mhws_head_id, int));
};

struct MockLeaf {
    MOCK_METHOD(bool, check_leaf, (const pud_node*));
};

struct MockChildren {
    MOCK_METHOD((const std::vector<const pud_node*>&), get, (const pud_node*));
};

struct MockPropagate {
    MOCK_METHOD(std::optional<int>, propagate, (int, const pud_node*));
};

using test_candidate_head_t = pud_candidate_search_head<
    int, child_iter, MockTryAdd, MockAdvance, MockForkWitness, MockLeaf, MockChildren, MockPropagate>;

struct PudCandidateSearchHeadTest : public ::testing::Test {
    NiceMock<MockTryAdd> try_add;
    NiceMock<MockAdvance> advance;
    NiceMock<MockForkWitness> fork_witness;
    NiceMock<MockLeaf> leaves;
    NiceMock<MockChildren> children;
    NiceMock<MockPropagate> propagate;
    pud_node root{};
    pud_node a{};
    pud_node b{};
    pud_node c{};
    pud_node a1{};
    pud_node a2{};
    std::unordered_map<const pud_node*, std::vector<const pud_node*>> sequences;

    test_candidate_head_t make_head(const pud_node* node, int handle) {
        return test_candidate_head_t{
            try_add, advance, fork_witness, leaves, children, propagate,
            pud_query_position<int>{.handle = handle, .node = node}};
    }

    void SetUp() override {
        ON_CALL(leaves, check_leaf(_)).WillByDefault(Return(false));
        ON_CALL(children, get(_)).WillByDefault([&](const pud_node* node) -> const std::vector<const pud_node*>& {
            return sequences.at(node);
        });
        ON_CALL(propagate, propagate(_, _)).WillByDefault([](int handle, const pud_node*) {
            return std::optional<int>{handle + 1};
        });
    }
};

TEST_F(PudCandidateSearchHeadTest, NoEdgeYieldsAWitness) {
    sequences[&root] = {&a, &b};
    EXPECT_CALL(try_add, try_add_head(_)).WillRepeatedly(Return(std::nullopt));
    auto head = make_head(&root, 1);
    EXPECT_FALSE(head.resume().has_value());
}

TEST_F(PudCandidateSearchHeadTest, FirstEdgeMissesThenTwoWitnessesFormChoicePoint) {
    sequences[&root] = {&a, &b, &c};
    EXPECT_CALL(try_add, try_add_head(_))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(pud_mhws_head_id{10}))
        .WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(&root, 1);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    auto* choice = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(choice, nullptr);
    EXPECT_EQ(choice->witness_a, 10u);
    EXPECT_EQ(choice->witness_b, 11u);
}

TEST_F(PudCandidateSearchHeadTest, ThirdWitnessIsNotPartOfTheChoicePoint) {
    sequences[&root] = {&a, &b, &c};
    EXPECT_CALL(try_add, try_add_head(_))
        .WillOnce(Return(pud_mhws_head_id{10}))
        .WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(&root, 1);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    auto* choice = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(choice, nullptr);
    EXPECT_EQ(choice->witness_a, 10u);
    EXPECT_EQ(choice->witness_b, 11u);
}

TEST_F(PudCandidateSearchHeadTest, SingleWitnessCausesAdvancementToSelfWitness) {
    sequences[&root] = {&a, &b};
    sequences[&b] = {};
    EXPECT_CALL(try_add, try_add_head(_))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(&b)).WillRepeatedly(Return(true));
    pud_query_frame<int, child_iter> frame{
        .position = pud_query_position<int>{.handle = 8, .node = &b},
        .next_child_it = {},
        .end_child_it = {}};
    EXPECT_CALL(advance, advance_head(10u)).WillOnce(Return(frame));
    auto head = make_head(&root, 1);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    auto* self = std::get_if<pud_candidate_self_witness>(&found->justification);
    ASSERT_NE(self, nullptr);
    EXPECT_EQ(self->node, &b);
    EXPECT_EQ(found->query_handle, 8);
}

TEST_F(PudCandidateSearchHeadTest, SingleWitnessCausesAdvancementToChoicePoint) {
    sequences[&root] = {&a};
    sequences[&a] = {&a1, &a2};
    EXPECT_CALL(try_add, try_add_head(_))
        .WillOnce(Return(pud_mhws_head_id{4}))
        .WillOnce(Return(pud_mhws_head_id{20}));
    pud_query_frame<int, child_iter> frame{
        .position = pud_query_position<int>{.handle = 6, .node = &a},
        .next_child_it = sequences[&a].begin(),
        .end_child_it = sequences[&a].end()};
    EXPECT_CALL(advance, advance_head(4u)).WillOnce(Return(frame));
    auto head = make_head(&root, 1);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    auto* choice = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(choice, nullptr);
    EXPECT_EQ(choice->witness_a, 4u);
    EXPECT_EQ(choice->witness_b, 20u);
}

TEST_F(PudCandidateSearchHeadTest, RefutedWitnessIsReplacedByALaterEdge) {
    sequences[&root] = {&a, &b, &c};
    EXPECT_CALL(try_add, try_add_head(_))
        .WillOnce(Return(pud_mhws_head_id{10}))
        .WillOnce(Return(pud_mhws_head_id{11}))
        .WillOnce(Return(pud_mhws_head_id{12}));
    auto head = make_head(&root, 1);
    auto first = head.resume();
    ASSERT_TRUE(first.has_value());
    head.witness_refuted(10);
    auto next = head.resume();
    ASSERT_TRUE(next.has_value());
    auto* choice = std::get_if<pud_candidate_choice_point>(&next->justification);
    ASSERT_NE(choice, nullptr);
    EXPECT_EQ(choice->witness_a, 12u);
    EXPECT_EQ(choice->witness_b, 11u);
}

TEST_F(PudCandidateSearchHeadTest, RefutedWitnessCausesAdvancementToSelfWitness) {
    sequences[&root] = {&a, &b};
    sequences[&b] = {};
    EXPECT_CALL(try_add, try_add_head(_))
        .WillOnce(Return(pud_mhws_head_id{10}))
        .WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(&root, 1);
    ASSERT_TRUE(head.resume().has_value());
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(&b)).WillRepeatedly(Return(true));
    pud_query_frame<int, child_iter> frame{
        .position = pud_query_position<int>{.handle = 9, .node = &b},
        .next_child_it = {},
        .end_child_it = {}};
    EXPECT_CALL(advance, advance_head(11u)).WillOnce(Return(frame));
    head.witness_refuted(10);
    auto next = head.resume();
    ASSERT_TRUE(next.has_value());
    auto* self = std::get_if<pud_candidate_self_witness>(&next->justification);
    ASSERT_NE(self, nullptr);
    EXPECT_EQ(self->node, &b);
}

TEST_F(PudCandidateSearchHeadTest, RefuteBothWitnessesWithNothingLeft) {
    sequences[&root] = {&a, &b};
    EXPECT_CALL(try_add, try_add_head(_))
        .WillOnce(Return(pud_mhws_head_id{10}))
        .WillOnce(Return(pud_mhws_head_id{11}))
        .WillRepeatedly(Return(std::nullopt));
    auto head = make_head(&root, 1);
    ASSERT_TRUE(head.resume().has_value());
    head.witness_refuted(10);
    head.witness_refuted(11);
    EXPECT_FALSE(head.resume().has_value());
}

TEST_F(PudCandidateSearchHeadTest, ForkThatCannotFollowThePathHasNoJustification) {
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(true));
    auto head = make_head(&root, 1);
    ASSERT_TRUE(head.resume().has_value());
    EXPECT_CALL(propagate, propagate(50, &root)).WillOnce(Return(std::nullopt));
    test_candidate_head_t forked{head, 50};
    EXPECT_FALSE(forked.resume().has_value());
}

TEST_F(PudCandidateSearchHeadTest, LeafRootIsASelfWitness) {
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(true));
    auto head = make_head(&root, 7);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    auto* self = std::get_if<pud_candidate_self_witness>(&found->justification);
    ASSERT_NE(self, nullptr);
    EXPECT_EQ(self->node, &root);
    EXPECT_EQ(found->query_handle, 7);
}

TEST_F(PudCandidateSearchHeadTest, TwoWitnessesFormAChoicePoint) {
    sequences[&root] = {&a, &b};
    EXPECT_CALL(try_add, try_add_head(_))
        .WillOnce(Return(pud_mhws_head_id{10}))
        .WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(&root, 1);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    auto* choice = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(choice, nullptr);
    EXPECT_EQ(choice->witness_a, 10u);
    EXPECT_EQ(choice->witness_b, 11u);
    EXPECT_EQ(found->query_handle, 1);
}
