#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_witness_search_head_forker.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

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
struct MockPropagate {
    MOCK_METHOD(std::optional<handle_t>, propagate, (handle_t, pud_node_id));
};
struct MockCallSite {
    MOCK_METHOD(size_t, get, (pud_node_id));
};

using test_head_t = pud_witness_search_head<
    handle_t, child_iter, MockCheckLeaf, MockGetChildren, MockPropagate, MockCallSite>;

struct PudWitnessSearchHeadForkerTest : public ::testing::Test {
    NiceMock<MockCheckLeaf> leaves;
    NiceMock<MockGetChildren> children;
    NiceMock<MockPropagate> propagate;
    NiceMock<MockCallSite> call_sites;
    pud_witness_search_head_forker<handle_t, child_iter, MockCheckLeaf, MockGetChildren, MockPropagate, MockCallSite> forker;
    pud_node_id root = 1;
    pud_node_id parent_node = 2;
    pud_node_id leaf = 3;
    pud_node_id next = 4;
    pud_node_id caller = 5;
    std::vector<pud_node_id> root_children{parent_node};
    std::vector<pud_node_id> parent_children{leaf, next};

    test_head_t make_head(pud_node_id node) {
        return test_head_t{
            leaves, children, propagate, call_sites,
            handle_t{node}};
    }
};

TEST_F(PudWitnessSearchHeadForkerTest, ForkContinuesWithParentsNextChild) {
    EXPECT_CALL(leaves, check_leaf(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(leaves, check_leaf(next)).WillRepeatedly(Return(true));
    EXPECT_CALL(children, get(root)).WillRepeatedly(ReturnRef(root_children));
    EXPECT_CALL(children, get(parent_node)).WillRepeatedly(ReturnRef(parent_children));
    EXPECT_CALL(propagate, propagate(_, parent_node)).WillOnce(Return(std::optional<handle_t>{handle_t{parent_node}}));
    EXPECT_CALL(propagate, propagate(_, leaf)).WillOnce(Return(std::optional<handle_t>{handle_t{leaf}}));
    auto head = make_head(root);
    ASSERT_EQ(head.resume().value_or(pud_node_id{0}), leaf);
    // fork goes directly to the first frame node (parent_node), not via root
    EXPECT_CALL(propagate, propagate(handle_t{caller}, parent_node)).WillOnce(Return(std::optional<handle_t>{handle_t{parent_node}}));
    EXPECT_CALL(propagate, propagate(handle_t{parent_node}, leaf)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(propagate, propagate(handle_t{parent_node}, next)).WillOnce(Return(std::optional<handle_t>{handle_t{next}}));
    auto forked = forker.fork(head, handle_t{caller});
    auto found = forked.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, next);
}

TEST_F(PudWitnessSearchHeadForkerTest, ForkThatCannotEnterRootHasNoLeaf) {
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(true));
    auto head = make_head(root);
    ASSERT_EQ(head.resume().value_or(pud_node_id{0}), root);
    EXPECT_CALL(propagate, propagate(handle_t{caller}, root)).WillOnce(Return(std::nullopt));
    auto forked = forker.fork(head, handle_t{caller});
    EXPECT_FALSE(forked.resume().has_value());
}

TEST_F(PudWitnessSearchHeadForkerTest, ForkOfALeafReportsThatLeaf) {
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(true));
    auto head = make_head(root);
    EXPECT_CALL(propagate, propagate(handle_t{caller}, root)).WillOnce(Return(std::optional<handle_t>{handle_t{root}}));
    auto forked = forker.fork(head, handle_t{caller});
    auto found = forked.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, root);
}
