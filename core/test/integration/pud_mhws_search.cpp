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

using head_t    = pud_witness_search_head<handle_t, child_iter, MockCheckLeaf, MockGetChildren, MockDescend, MockCallSite>;
using factory_t = pud_witness_search_head_factory<handle_t, child_iter, MockCheckLeaf, MockGetChildren, MockDescend, MockCallSite>;
using forker_t  = pud_witness_search_head_forker<handle_t, child_iter, MockCheckLeaf, MockGetChildren, MockDescend, MockCallSite>;
using mhws_t    = pud_mhws<handle_t, child_iter, head_t, factory_t, forker_t>;

struct PudMhwsSearchIntegrationTest : public ::testing::Test {
    NiceMock<MockCheckLeaf>  leaves;
    NiceMock<MockGetChildren> children;
    NiceMock<MockDescend>  descend;
    NiceMock<MockCallSite>   call_sites;
    factory_t factory{leaves, children, descend, call_sites};
    forker_t  forker;
    mhws_t    searches{factory, forker};
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> sequences;
    pud_node_id root         = 1;
    pud_node_id left         = 2;
    pud_node_id right        = 3;
    pud_node_id leaf         = 4;
    pud_node_id other_leaf   = 5;
    pud_node_id dead         = 6;
    pud_node_id deep1        = 7;
    pud_node_id deep2        = 8;
    pud_node_id deep3        = 9;
    pud_node_id sibling_leaf = 10;
    pud_node_id fork_caller  = 11;

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

TEST_F(PudMhwsSearchIntegrationTest, AllPropagationsBlockedReturnsNoHead) {
    sequences[root] = {left, right};
    sequences[left] = {};
    sequences[right] = {};
    EXPECT_CALL(descend, descend(_, _)).WillRepeatedly(Return(std::nullopt));
    EXPECT_FALSE(searches.try_add_head(handle_t{root}).has_value());
}

TEST_F(PudMhwsSearchIntegrationTest, InvalidateOfWitnessWithDeadSiblingSubtreeMovesToSibling) {
    sequences[root]  = {left, right};
    sequences[left]  = {dead, leaf};
    sequences[dead]  = {deep1};
    sequences[deep1] = {};
    sequences[leaf]  = {};
    sequences[right] = {};
    ON_CALL(leaves, check_leaf(leaf)).WillByDefault(Return(true));
    ON_CALL(descend, descend(_, deep1)).WillByDefault(Return(std::nullopt));
    auto id = searches.try_add_head(handle_t{root});
    ASSERT_TRUE(id.has_value());

    // leaf is no longer a leaf, and nothing else is reachable (all propagations now return nullopt)
    ON_CALL(leaves, check_leaf(leaf)).WillByDefault(Return(false));
    ON_CALL(descend, descend(_, _)).WillByDefault(Return(std::nullopt));
    auto gone = searches.invalidate_leaf(leaf);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *id);
}

TEST_F(PudMhwsSearchIntegrationTest, ForkSucceedsWhenChildIsReachable) {
    sequences[root]   = {deep1};
    sequences[deep1]  = {deep2};
    sequences[deep2]  = {deep3};
    sequences[deep3]  = {};
    ON_CALL(leaves, check_leaf(deep3)).WillByDefault(Return(true));
    auto id = searches.try_add_head(handle_t{root});
    ASSERT_TRUE(id.has_value());
    auto forked = searches.try_fork_head(*id, handle_t{fork_caller});
    ASSERT_TRUE(forked.has_value());
    EXPECT_NE(forked.value(), *id);
}

TEST_F(PudMhwsSearchIntegrationTest, ForkThatCannotEnterDeepestNodeReturnsNoForked) {
    sequences[root]   = {deep1};
    sequences[deep1]  = {deep2};
    sequences[deep2]  = {deep3};
    sequences[deep3]  = {};
    ON_CALL(leaves, check_leaf(deep3)).WillByDefault(Return(true));
    auto id = searches.try_add_head(handle_t{root});
    ASSERT_TRUE(id.has_value());

    ON_CALL(descend, descend(_, deep3)).WillByDefault(Return(std::nullopt));
    EXPECT_FALSE(searches.try_fork_head(*id, handle_t{fork_caller}).has_value());

    // original head still points to deep3; invalidate it (deep3 no longer a leaf)
    ON_CALL(leaves, check_leaf(deep3)).WillByDefault(Return(false));
    auto gone = searches.invalidate_leaf(deep3);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *id);
}

TEST_F(PudMhwsSearchIntegrationTest, InvalidateLeafMovesHeadToNextLeafUnderSameParent) {
    sequences[root]  = {deep1};
    sequences[deep1] = {deep2};
    sequences[deep2] = {leaf, sibling_leaf};
    sequences[leaf]  = {};
    sequences[sibling_leaf] = {};
    ON_CALL(leaves, check_leaf(leaf)).WillByDefault(Return(true));
    ON_CALL(leaves, check_leaf(sibling_leaf)).WillByDefault(Return(true));
    auto id = searches.try_add_head(handle_t{root});
    ASSERT_TRUE(id.has_value());

    // leaf is invalidated; head should move to sibling_leaf
    ON_CALL(leaves, check_leaf(leaf)).WillByDefault(Return(false));
    auto moved = searches.invalidate_leaf(leaf);
    EXPECT_TRUE(moved.empty());

    // sibling_leaf is also invalidated; head loses witness
    ON_CALL(leaves, check_leaf(sibling_leaf)).WillByDefault(Return(false));
    auto gone = searches.invalidate_leaf(sibling_leaf);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *id);
}
