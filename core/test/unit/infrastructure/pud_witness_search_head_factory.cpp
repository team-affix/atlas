#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_witness_search_head_factory.hpp"

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

using test_factory_t = pud_witness_search_head_factory<
    handle_t, child_iter, MockCheckLeaf, MockGetChildren, MockPropagate, MockCallSite>;

struct PudWitnessSearchHeadFactoryTest : public ::testing::Test {
    NiceMock<MockCheckLeaf> leaves;
    NiceMock<MockGetChildren> children;
    NiceMock<MockPropagate> propagate;
    NiceMock<MockCallSite> call_sites;
    test_factory_t factory{leaves, children, propagate, call_sites};
    pud_node root{};
    pud_node child{};
    pud_node grand{};
    std::vector<const pud_node*> root_children{&child};
    std::vector<const pud_node*> child_children{&grand};
    std::vector<const pud_node*> grand_children{};
};

TEST_F(PudWitnessSearchHeadFactoryTest, NoLeafIsReachable) {
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(&child)).WillRepeatedly(Return(false));
    EXPECT_CALL(children, get(&root)).WillRepeatedly(ReturnRef(root_children));
    EXPECT_CALL(children, get(&child)).WillRepeatedly(ReturnRef(child_children));
    EXPECT_CALL(children, get(&grand)).WillRepeatedly(ReturnRef(grand_children));
    EXPECT_CALL(propagate, propagate(_, _)).WillRepeatedly(Return(std::nullopt));
    auto head = factory.make(handle_t{&root});
    EXPECT_FALSE(head.resume().has_value());
}

TEST_F(PudWitnessSearchHeadFactoryTest, LeafRootIsThatNode) {
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(true));
    auto head = factory.make(handle_t{&root});
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, &root);
}
