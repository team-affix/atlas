#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_witness_search_head_forker.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

using child_iter = std::vector<const pud_node*>::const_iterator;

struct handle_t {
    const pud_node* node_ptr;
    const pud_node* node() const { return node_ptr; }
    bool operator==(const handle_t&) const = default;
};

struct MockCheckLeaf {
    MOCK_METHOD(bool, check_leaf, (const pud_node*));
};
struct MockGetChildren {
    MOCK_METHOD((const std::vector<const pud_node*>&), get, (const pud_node*));
};
struct MockPropagate {
    MOCK_METHOD(std::optional<handle_t>, propagate, (handle_t, const pud_node*));
};
struct MockCallSite {
    MOCK_METHOD(size_t, get, (const pud_node*));
};

using test_head_t = pud_witness_search_head<
    handle_t, child_iter, MockCheckLeaf, MockGetChildren, MockPropagate, MockCallSite>;

struct PudWitnessSearchHeadForkerTest : public ::testing::Test {
    NiceMock<MockCheckLeaf> leaves;
    NiceMock<MockGetChildren> children;
    NiceMock<MockPropagate> propagate;
    NiceMock<MockCallSite> call_sites;
    pud_witness_search_head_forker<handle_t, child_iter, MockCheckLeaf, MockGetChildren, MockPropagate, MockCallSite> forker;
    pud_node root{};
    pud_node parent{};
    pud_node leaf{};
    pud_node next{};
    pud_node caller{};
    std::vector<const pud_node*> root_children{&parent};
    std::vector<const pud_node*> parent_children{&leaf, &next};

    test_head_t make_head(const pud_node* node) {
        return test_head_t{
            leaves, children, propagate, call_sites,
            handle_t{node}};
    }
};

TEST_F(PudWitnessSearchHeadForkerTest, ForkContinuesWithParentsNextChild) {
    EXPECT_CALL(leaves, check_leaf(&leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(leaves, check_leaf(&next)).WillRepeatedly(Return(true));
    EXPECT_CALL(children, get(&root)).WillRepeatedly(ReturnRef(root_children));
    EXPECT_CALL(children, get(&parent)).WillRepeatedly(ReturnRef(parent_children));
    EXPECT_CALL(propagate, propagate(_, &parent)).WillOnce(Return(std::optional<handle_t>{handle_t{&parent}}));
    EXPECT_CALL(propagate, propagate(_, &leaf)).WillOnce(Return(std::optional<handle_t>{handle_t{&leaf}}));
    auto head = make_head(&root);
    ASSERT_EQ(head.resume().value_or(nullptr), &leaf);
    EXPECT_CALL(propagate, propagate(handle_t{&caller}, &root)).WillOnce(Return(std::optional<handle_t>{handle_t{&root}}));
    EXPECT_CALL(propagate, propagate(handle_t{&root}, &parent)).WillOnce(Return(std::optional<handle_t>{handle_t{&parent}}));
    EXPECT_CALL(propagate, propagate(handle_t{&parent}, &leaf)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(propagate, propagate(handle_t{&parent}, &next)).WillOnce(Return(std::optional<handle_t>{handle_t{&next}}));
    auto forked = forker.fork(head, handle_t{&caller});
    auto found = forked.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, &next);
}

TEST_F(PudWitnessSearchHeadForkerTest, ForkThatCannotEnterRootHasNoLeaf) {
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(true));
    auto head = make_head(&root);
    ASSERT_EQ(head.resume().value_or(nullptr), &root);
    EXPECT_CALL(propagate, propagate(handle_t{&caller}, &root)).WillOnce(Return(std::nullopt));
    auto forked = forker.fork(head, handle_t{&caller});
    EXPECT_FALSE(forked.resume().has_value());
}

TEST_F(PudWitnessSearchHeadForkerTest, ForkOfALeafReportsThatLeaf) {
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(true));
    auto head = make_head(&root);
    EXPECT_CALL(propagate, propagate(handle_t{&caller}, &root)).WillOnce(Return(std::optional<handle_t>{handle_t{&root}}));
    auto forked = forker.fork(head, handle_t{&caller});
    auto found = forked.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, &root);
}
