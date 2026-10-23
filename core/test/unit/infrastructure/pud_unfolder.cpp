#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unfolder.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockStoreCallSite {
    MOCK_METHOD(void, store, (pud_node_id, pud_body_goal_id));
};
struct MockMakeChild {
    MOCK_METHOD(pud_node_id, make_child, (pud_node_id, pud_body_goal_id, const pud_child_delta&));
};
struct MockStoreChildren {
    MOCK_METHOD(void, store, (pud_node_id, std::vector<pud_node_id>));
};
struct MockMarkNodeState {
    MOCK_METHOD(void, unset_leaf, (pud_node_id));
};
struct MockStoreLiveChildCount {
    MOCK_METHOD(void, store, (pud_node_id, size_t));
};
struct MockPropagateRefutation {
    MOCK_METHOD(void, refute, (pud_node_id));
};

using test_unfolder_t = pud_unfolder<
    NiceMock<MockStoreCallSite>, NiceMock<MockMakeChild>, NiceMock<MockStoreChildren>, NiceMock<MockMarkNodeState>,
    NiceMock<MockStoreLiveChildCount>, NiceMock<MockPropagateRefutation>>;

struct PudUnfolderTest : public ::testing::Test {
    NiceMock<MockStoreCallSite> call_sites;
    NiceMock<MockMakeChild> make_child;
    NiceMock<MockStoreChildren> children;
    NiceMock<MockMarkNodeState> states;
    NiceMock<MockStoreLiveChildCount> live_child_counts;
    NiceMock<MockPropagateRefutation> propagator;
    test_unfolder_t unfolder{call_sites, make_child, children, states, live_child_counts, propagator};
    pud_child_delta first{.substitutions = {}, .body_goals = {}, .var_count = 1};
    pud_child_delta second{.substitutions = {}, .body_goals = {}, .var_count = 2};
};

}

TEST_F(PudUnfolderTest, TheCallSiteIsStoredBeforeAnyChildIsMade) {
    InSequence in_order;
    EXPECT_CALL(call_sites, store(4, 2));
    EXPECT_CALL(make_child, make_child(4, 2, first)).WillOnce(Return(10));
    EXPECT_CALL(make_child, make_child(4, 2, second)).WillOnce(Return(11));
    unfolder.begin_unfold(4, 2);
    EXPECT_EQ(unfolder.unfold_child(4, 2, first), 10u);
    EXPECT_EQ(unfolder.unfold_child(4, 2, second), 11u);
    unfolder.finish_unfold(4, {10, 11});
}

TEST_F(PudUnfolderTest, TheLeafBecomesInteriorWithEveryChildAlive) {
    ON_CALL(make_child, make_child(_, _, first)).WillByDefault(Return(10));
    ON_CALL(make_child, make_child(_, _, second)).WillByDefault(Return(11));
    EXPECT_CALL(children, store(4, std::vector<pud_node_id>{10, 11}));
    EXPECT_CALL(states, unset_leaf(4));
    EXPECT_CALL(live_child_counts, store(4, 2u));
    EXPECT_CALL(propagator, refute(_)).Times(0);
    unfolder.begin_unfold(4, 2);
    unfolder.unfold_child(4, 2, first);
    unfolder.unfold_child(4, 2, second);
    unfolder.finish_unfold(4, {10, 11});
}

TEST_F(PudUnfolderTest, ALeafWithoutChildrenIsDead) {
    EXPECT_CALL(live_child_counts, store(4, 0u));
    EXPECT_CALL(propagator, refute(4));
    unfolder.begin_unfold(4, 2);
    unfolder.finish_unfold(4, {});
}
