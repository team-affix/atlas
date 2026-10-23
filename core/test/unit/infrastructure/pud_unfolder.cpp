#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unfolder.hpp"

using ::testing::NiceMock;

namespace {

struct MockStoreCallSite {
    MOCK_METHOD(void, store, (pud_node_id, pud_body_goal_id));
};
struct MockStoreChildren {
    MOCK_METHOD(void, store, (pud_node_id, std::vector<pud_node_id>));
};
struct MockUnsetLeaf {
    MOCK_METHOD(void, unset_leaf, (pud_node_id));
};
struct MockStoreLiveChildCount {
    MOCK_METHOD(void, store, (pud_node_id, size_t));
};

using test_unfolder_t = pud_unfolder<
    NiceMock<MockStoreCallSite>, NiceMock<MockStoreChildren>, NiceMock<MockUnsetLeaf>, NiceMock<MockStoreLiveChildCount>>;

struct PudUnfolderTest : public ::testing::Test {
    NiceMock<MockStoreCallSite> call_sites;
    NiceMock<MockStoreChildren> children;
    NiceMock<MockUnsetLeaf> leaves;
    NiceMock<MockStoreLiveChildCount> live_child_counts;
    test_unfolder_t unfolder{call_sites, children, leaves, live_child_counts};
};

}

TEST_F(PudUnfolderTest, TheLeafBecomesInteriorOnItsCallSiteWithEveryChildAlive) {
    EXPECT_CALL(call_sites, store(4, 2));
    EXPECT_CALL(children, store(4, std::vector<pud_node_id>{10, 11}));
    EXPECT_CALL(leaves, unset_leaf(4));
    EXPECT_CALL(live_child_counts, store(4, 2u));
    unfolder.unfold(4, 2, {10, 11});
}
