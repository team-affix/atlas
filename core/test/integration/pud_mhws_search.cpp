#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_witness_search_head_factory.hpp"
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

using head_t = pud_witness_search_head<
    handle_t, child_iter, MockCheckLeaf, MockGetChildren, MockPropagate, MockCallSite>;
using factory_t = pud_witness_search_head_factory<
    handle_t, child_iter, MockCheckLeaf, MockGetChildren, MockPropagate, MockCallSite>;
using forker_t = pud_witness_search_head_forker<
    handle_t, child_iter, MockCheckLeaf, MockGetChildren, MockPropagate, MockCallSite>;
using mhws_t = pud_mhws<handle_t, child_iter, head_t, factory_t, forker_t>;

struct PudMhwsSearchIntegrationTest : public ::testing::Test {
    NiceMock<MockCheckLeaf> leaves;
    NiceMock<MockGetChildren> children;
    NiceMock<MockPropagate> propagate;
    NiceMock<MockCallSite> call_sites;
    factory_t factory{leaves, children, propagate, call_sites};
    forker_t forker;
    mhws_t searches{factory, forker};
    std::unordered_map<const pud_node*, std::vector<const pud_node*>> sequences;
    pud_node root{};
    pud_node left{};
    pud_node right{};
    pud_node dead{};
    pud_node leaf{};
    pud_node other_leaf{};
    pud_node deep1{};
    pud_node deep2{};
    pud_node deep3{};
    pud_node sibling_leaf{};
    pud_node fork_caller{};

    void SetUp() override {
        ON_CALL(leaves, check_leaf(_)).WillByDefault(Return(false));
        ON_CALL(children, get(_)).WillByDefault([&](const pud_node* node) -> const std::vector<const pud_node*>& {
            return sequences.at(node);
        });
        ON_CALL(propagate, propagate(_, _)).WillByDefault([](handle_t, const pud_node* child) {
            return std::optional<handle_t>{handle_t{child}};
        });
    }
};

TEST_F(PudMhwsSearchIntegrationTest, NoReachableLeafIsNotRemembered) {
    sequences[&root] = {&left, &right};
    EXPECT_CALL(propagate, propagate(_, _)).WillRepeatedly(Return(std::nullopt));
    EXPECT_FALSE(searches.try_add_head(handle_t{&root}).has_value());
    EXPECT_TRUE(searches.invalidate_leaf(&root).empty());
}

TEST_F(PudMhwsSearchIntegrationTest, InvalidateOfDeepWitnessWithADeadSiblingSubtree) {
    sequences[&root] = {&left, &right};
    sequences[&left] = {&dead, &leaf};
    sequences[&dead] = {&deep1};
    sequences[&deep1] = {};
    EXPECT_CALL(leaves, check_leaf(&leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(propagate, propagate(_, &deep1)).WillRepeatedly(Return(std::nullopt));
    auto id = searches.try_add_head(handle_t{&root});
    ASSERT_TRUE(id.has_value());
    EXPECT_CALL(leaves, check_leaf(&leaf)).WillRepeatedly(Return(false));
    EXPECT_CALL(propagate, propagate(_, _)).WillRepeatedly(Return(std::nullopt));
    auto gone = searches.invalidate_leaf(&leaf);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *id);
    EXPECT_EQ(sequences[&root][1], &right);
    EXPECT_EQ(sequences[&left][0], &dead);
    EXPECT_EQ(sequences[&left][1], &leaf);
}

TEST_F(PudMhwsSearchIntegrationTest, ForkThatCannotEnterTheDeepestNodeIsNotRemembered) {
    sequences[&root] = {&deep1};
    sequences[&deep1] = {&deep2};
    sequences[&deep2] = {&deep3};
    EXPECT_CALL(leaves, check_leaf(&deep3)).WillRepeatedly(Return(true));
    auto id = searches.try_add_head(handle_t{&root});
    ASSERT_TRUE(id.has_value());
    EXPECT_CALL(propagate, propagate(_, &deep3)).WillOnce(Return(std::nullopt));
    EXPECT_FALSE(searches.try_fork_head(*id, handle_t{&fork_caller}).has_value());
    EXPECT_TRUE(searches.invalidate_leaf(&deep2).empty());
    auto original = searches.invalidate_leaf(&deep3);
    ASSERT_EQ(original.size(), 1u);
    EXPECT_EQ(original[0], *id);
}

TEST_F(PudMhwsSearchIntegrationTest, DepthFourLeafMovesToTheOtherLeafUnderTheSameParent) {
    sequences[&root] = {&deep1};
    sequences[&deep1] = {&deep2};
    sequences[&deep2] = {&leaf, &sibling_leaf};
    EXPECT_CALL(leaves, check_leaf(&leaf)).WillRepeatedly(Return(true));
    EXPECT_CALL(leaves, check_leaf(&sibling_leaf)).WillRepeatedly(Return(true));
    auto id = searches.try_add_head(handle_t{&root});
    ASSERT_TRUE(id.has_value());
    EXPECT_CALL(leaves, check_leaf(&leaf)).WillRepeatedly(Return(false));
    EXPECT_TRUE(searches.invalidate_leaf(&leaf).empty());
    EXPECT_TRUE(searches.invalidate_leaf(&leaf).empty());
    auto moved = searches.invalidate_leaf(&sibling_leaf);
    ASSERT_EQ(moved.size(), 1u);
    EXPECT_EQ(moved[0], *id);
}
