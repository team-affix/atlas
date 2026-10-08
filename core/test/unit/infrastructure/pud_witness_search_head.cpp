#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_witness_search_head.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

using child_iter = std::vector<pud_node_id>::const_iterator;

struct handle_t {
    pud_node_id node_id;
    pud_node_id node() const { return node_id; }
    bool operator==(const handle_t&) const = default;
};

struct MockCheckLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_node_id));
};

struct MockGetChildren {
    MOCK_METHOD((const std::vector<pud_node_id>&), get, (pud_node_id));
};

struct MockDescend {
    MOCK_METHOD(std::optional<handle_t>, descend, (handle_t, pud_node_id));
};

struct MockCallSite {
    MOCK_METHOD(size_t, get, (pud_node_id));
};

using test_witness_head_t = pud_witness_search_head<
    handle_t,
    child_iter,
    MockCheckLeaf,
    MockGetChildren,
    MockDescend,
    MockCallSite>;

struct PudWitnessSearchHeadTest : public ::testing::Test {
    NiceMock<MockCheckLeaf> leaves;
    NiceMock<MockGetChildren> children;
    NiceMock<MockDescend> descend;
    NiceMock<MockCallSite> call_sites;
    pud_node_id root    = 1;
    pud_node_id left    = 2;
    pud_node_id right   = 3;
    pud_node_id left_1  = 4;
    pud_node_id left_2  = 5;
    pud_node_id deep_a  = 6;
    pud_node_id deep_b  = 7;
    pud_node_id deep_c  = 8;
    pud_node_id deep_leaf = 9;
    pud_node_id wide_0  = 10;
    pud_node_id wide_1  = 11;
    pud_node_id wide_2  = 12;
    pud_node_id wide_3  = 13;
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> sequences;

    test_witness_head_t make_head(pud_node_id node) {
        return test_witness_head_t{
            leaves, children, descend, call_sites,
            handle_t{node}};
    }

    void SetUp() override {
        ON_CALL(leaves, check_leaf(_)).WillByDefault(Return(false));
        ON_CALL(children, get(_)).WillByDefault([&](pud_node_id id) -> const std::vector<pud_node_id>& {
            return sequences.at(id);
        });
        ON_CALL(descend, descend(_, _)).WillByDefault([](handle_t, pud_node_id child) {
            return std::optional<handle_t>{handle_t{child}};
        });
    }
};

TEST_F(PudWitnessSearchHeadTest, NoLeafIsReachable) {
    sequences[root] = {left};
    sequences[left] = {};
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left)).WillRepeatedly(Return(false));
    EXPECT_CALL(descend, descend(_, left)).WillRepeatedly(Return(std::nullopt));
    auto head = make_head(root);
    EXPECT_FALSE(head.resume().has_value());
}

TEST_F(PudWitnessSearchHeadTest, FirstChildRefusedAndSiblingIsTheLeaf) {
    sequences[root] = {left, right};
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(right)).WillRepeatedly(Return(true));
    EXPECT_CALL(descend, descend(_, _))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::optional<handle_t>{handle_t{right}}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, right);
}

TEST_F(PudWitnessSearchHeadTest, LaterSiblingIsTheLeaf) {
    sequences[root] = {left, right};
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(right)).WillRepeatedly(Return(true));
    EXPECT_CALL(descend, descend(_, left)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(descend, descend(_, right)).WillOnce(Return(std::optional<handle_t>{handle_t{right}}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, right);
}

TEST_F(PudWitnessSearchHeadTest, DeadSubtreeReturnsParentsNextChild) {
    sequences[root]   = {left, right};
    sequences[left]   = {left_1, left_2};
    sequences[left_1] = {deep_a, deep_b};
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left_1)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left_2)).WillRepeatedly(Return(true));
    EXPECT_CALL(descend, descend(_, left)).WillOnce(Return(std::optional<handle_t>{handle_t{left}}));
    EXPECT_CALL(descend, descend(_, left_1)).WillOnce(Return(std::optional<handle_t>{handle_t{left_1}}));
    EXPECT_CALL(descend, descend(_, deep_a)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(descend, descend(_, deep_b)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(descend, descend(_, left_2)).WillOnce(Return(std::optional<handle_t>{handle_t{left_2}}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, left_2);
}

TEST_F(PudWitnessSearchHeadTest, DeepLeafBeforeShallowSibling) {
    sequences[root]   = {left, right};
    sequences[left]   = {left_1, left_2};
    sequences[left_1] = {deep_a, deep_leaf};
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left_1)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(deep_leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(descend, descend(_, left)).WillOnce(Return(std::optional<handle_t>{handle_t{left}}));
    EXPECT_CALL(descend, descend(_, left_1)).WillOnce(Return(std::optional<handle_t>{handle_t{left_1}}));
    EXPECT_CALL(descend, descend(_, deep_a)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(descend, descend(_, deep_leaf)).WillOnce(Return(std::optional<handle_t>{handle_t{deep_leaf}}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, deep_leaf);
}

TEST_F(PudWitnessSearchHeadTest, WideNodeThenDeepLeaf) {
    sequences[root]   = {wide_0, wide_1, wide_2, wide_3};
    sequences[wide_3] = {deep_a};
    sequences[deep_a] = {deep_b};
    sequences[deep_b] = {deep_leaf};
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(wide_3)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(deep_a)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(deep_b)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(deep_leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(descend, descend(_, wide_0)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(descend, descend(_, wide_1)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(descend, descend(_, wide_2)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(descend, descend(_, wide_3)).WillOnce(Return(std::optional<handle_t>{handle_t{wide_3}}));
    EXPECT_CALL(descend, descend(_, deep_a)).WillOnce(Return(std::optional<handle_t>{handle_t{deep_a}}));
    EXPECT_CALL(descend, descend(_, deep_b)).WillOnce(Return(std::optional<handle_t>{handle_t{deep_b}}));
    EXPECT_CALL(descend, descend(_, deep_leaf)).WillOnce(Return(std::optional<handle_t>{handle_t{deep_leaf}}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, deep_leaf);
}

TEST_F(PudWitnessSearchHeadTest, DeepBranchDiesAndOtherBranchLeafIsFound) {
    sequences[root]   = {left, right};
    sequences[left]   = {left_1};
    sequences[left_1] = {deep_a};
    sequences[deep_a] = {deep_b};
    sequences[right]  = {deep_leaf};
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left_1)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(deep_a)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(right)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(deep_leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(descend, descend(_, left)).WillOnce(Return(std::optional<handle_t>{handle_t{left}}));
    EXPECT_CALL(descend, descend(_, left_1)).WillOnce(Return(std::optional<handle_t>{handle_t{left_1}}));
    EXPECT_CALL(descend, descend(_, deep_a)).WillOnce(Return(std::optional<handle_t>{handle_t{deep_a}}));
    EXPECT_CALL(descend, descend(_, deep_b)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(descend, descend(_, right)).WillOnce(Return(std::optional<handle_t>{handle_t{right}}));
    EXPECT_CALL(descend, descend(_, deep_leaf)).WillOnce(Return(std::optional<handle_t>{handle_t{deep_leaf}}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, deep_leaf);
}

TEST_F(PudWitnessSearchHeadTest, AdvanceReturnsFirstChildHandleAndRootSiblingIterators) {
    sequences[root] = {left, right};
    sequences[left] = {deep_leaf};
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(deep_leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(descend, descend(_, left)).WillOnce(Return(std::optional<handle_t>{handle_t{left}}));
    EXPECT_CALL(descend, descend(_, deep_leaf)).WillOnce(Return(std::optional<handle_t>{handle_t{deep_leaf}}));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    auto result = head.advance();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->root_handle.node(), left);
    EXPECT_EQ(*result->root_next_sibling_it, right);
    EXPECT_EQ(result->root_end_sibling_it, sequences[root].end());
}

TEST_F(PudWitnessSearchHeadTest, AdvanceThenResumeFindsLeafInRemainingFrame) {
    sequences[root] = {left, right};
    sequences[left] = {deep_leaf};
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(deep_leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(descend, descend(_, left)).WillOnce(Return(std::optional<handle_t>{handle_t{left}}));
    EXPECT_CALL(descend, descend(_, deep_leaf)).WillOnce(Return(std::optional<handle_t>{handle_t{deep_leaf}}));
    auto head = make_head(root);
    ASSERT_EQ(head.resume().value(), deep_leaf);
    auto advanced = head.advance();
    ASSERT_TRUE(advanced.has_value());
    EXPECT_EQ(advanced->root_handle.node(), left);
    // resume from the remaining frame — deep_leaf is still the active leaf
    EXPECT_EQ(head.resume().value(), deep_leaf);
}

TEST_F(PudWitnessSearchHeadTest, AdvanceAfterExhaustedSearchReturnsNullopt) {
    sequences[root] = {left};
    sequences[left] = {};
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left)).WillRepeatedly(Return(false));
    EXPECT_CALL(descend, descend(_, left)).WillOnce(Return(std::nullopt));
    auto head = make_head(root);
    ASSERT_FALSE(head.resume().has_value());
    EXPECT_FALSE(head.advance().has_value());
}

TEST_F(PudWitnessSearchHeadTest, ForkThatCannotEnterDeepestContinuesParentChildren) {
    sequences[root] = {left};
    sequences[left] = {left_1, left_2};
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left_1)).WillRepeatedly(Return(true));
    EXPECT_CALL(leaves, check_leaf(left_2)).WillRepeatedly(Return(true));
    EXPECT_CALL(descend, descend(_, left)).WillOnce(Return(std::optional<handle_t>{handle_t{left}}));
    EXPECT_CALL(descend, descend(_, left_1)).WillOnce(Return(std::optional<handle_t>{handle_t{left_1}}));
    auto head = make_head(root);
    ASSERT_EQ(head.resume().value(), left_1);
    EXPECT_CALL(descend, descend(handle_t{root}, left)).WillOnce(Return(std::optional<handle_t>{handle_t{left}}));
    EXPECT_CALL(descend, descend(handle_t{left}, left_1)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(descend, descend(handle_t{left}, left_2)).WillOnce(Return(std::optional<handle_t>{handle_t{left_2}}));
    test_witness_head_t forked{head, handle_t{root}};
    auto found = forked.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, left_2);
}

TEST_F(PudWitnessSearchHeadTest, ForkThatCannotEnterAnyChildHasNoLeaf) {
    sequences[root] = {left};
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left)).WillRepeatedly(Return(true));
    EXPECT_CALL(descend, descend(_, left)).WillOnce(Return(std::optional<handle_t>{handle_t{left}}));
    auto head = make_head(root);
    ASSERT_EQ(head.resume().value(), left);
    // fork's descend refuses to enter left — no siblings remain, so no leaf
    EXPECT_CALL(descend, descend(handle_t{root}, left)).WillOnce(Return(std::nullopt));
    test_witness_head_t forked{head, handle_t{root}};
    EXPECT_FALSE(forked.resume().has_value());
}

TEST_F(PudWitnessSearchHeadTest, LeafRootIsThatNode) {
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(true));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, root);
}

TEST_F(PudWitnessSearchHeadTest, ChainOfFourReachesTheLeaf) {
    sequences[root]   = {left};
    sequences[left]   = {left_1};
    sequences[left_1] = {deep_leaf};
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left_1)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(deep_leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(descend, descend(_, left)).WillOnce(Return(std::optional<handle_t>{handle_t{left}}));
    EXPECT_CALL(descend, descend(_, left_1)).WillOnce(Return(std::optional<handle_t>{handle_t{left_1}}));
    EXPECT_CALL(descend, descend(_, deep_leaf)).WillOnce(Return(std::optional<handle_t>{handle_t{deep_leaf}}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, deep_leaf);
}

TEST_F(PudWitnessSearchHeadTest, ExpandedLeafResumedFindsDeepChild) {
    sequences[root]   = {left};
    sequences[left]   = {left_1};
    sequences[left_1] = {deep_a, deep_b};
    EXPECT_CALL(leaves, check_leaf(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left_1))
        .WillOnce(Return(true))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(deep_a)).WillRepeatedly(Return(true));
    auto head = make_head(root);
    ASSERT_EQ(head.resume().value(), left_1);
    // left_1 expanded — next resume descends into its children
    EXPECT_EQ(head.resume().value(), deep_a);
}

TEST_F(PudWitnessSearchHeadTest, ExpandedLeafAfterAdvanceResumedFindsDeepChild) {
    sequences[root]   = {left};
    sequences[left]   = {left_1};
    sequences[left_1] = {deep_a, deep_b};
    EXPECT_CALL(leaves, check_leaf(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left_1))
        .WillOnce(Return(true))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(deep_a)).WillRepeatedly(Return(true));
    auto head = make_head(root);
    ASSERT_EQ(head.resume().value(), left_1);
    auto advanced = head.advance();
    ASSERT_TRUE(advanced.has_value());
    ASSERT_EQ(advanced->root_handle.node(), left);
    EXPECT_EQ(head.resume().value(), deep_a);
}

TEST_F(PudWitnessSearchHeadTest, ExpandedLeafWithDeadChildrenBacktracksToSibling) {
    sequences[root]   = {left};
    sequences[left]   = {left_1, left_2};
    sequences[left_1] = {deep_a, deep_b};
    EXPECT_CALL(leaves, check_leaf(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left_1))
        .WillOnce(Return(true))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(left_2)).WillRepeatedly(Return(true));
    ON_CALL(descend, descend(_, deep_a)).WillByDefault(Return(std::nullopt));
    ON_CALL(descend, descend(_, deep_b)).WillByDefault(Return(std::nullopt));
    auto head = make_head(root);
    ASSERT_EQ(head.resume().value(), left_1);
    auto advanced = head.advance();
    ASSERT_TRUE(advanced.has_value());
    ASSERT_EQ(advanced->root_handle.node(), left);
    // expanded left_1's children are all dead — resume backtracks to left_2
    EXPECT_EQ(head.resume().value(), left_2);
}
