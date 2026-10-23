#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_expander.hpp"
#include "value_objects/pud_child_delta.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockResolve {
    MOCK_METHOD(std::vector<pud_child_delta>, resolve, (pud_node_id, pud_body_goal_id));
};
struct MockUnfold {
    MOCK_METHOD(void, begin_unfold, (pud_node_id, pud_body_goal_id));
    MOCK_METHOD(pud_node_id, unfold_child, (pud_node_id, pud_body_goal_id, const pud_child_delta&));
    MOCK_METHOD(void, finish_unfold, (pud_node_id, const std::vector<pud_node_id>&));
};
struct MockActivateChild {
    MOCK_METHOD(bool, activate_child, (pud_node_id, pud_node_id));
};
struct MockRefuteLeaf {
    MOCK_METHOD(void, refute, (pud_node_id));
};
struct MockReleaseLeaf {
    MOCK_METHOD(void, release, (pud_node_id));
};
struct MockInvalidateQueryLeaf {
    MOCK_METHOD(std::vector<pud_mhq_head_id>, invalidate_leaf, (pud_node_id));
};
struct MockGetQueryHolders {
    MOCK_METHOD(std::vector<pud_node_id>, get_holders, (pud_mhq_head_id));
};
struct MockCollectChange {
    MOCK_METHOD(void, add_produced, (pud_node_id));
    MOCK_METHOD(void, add_refuted, (pud_node_id));
    MOCK_METHOD(pud_frontier_change, finish, ());
};

using test_expander_t = pud_expander<
    NiceMock<MockResolve>, NiceMock<MockUnfold>, NiceMock<MockActivateChild>, NiceMock<MockRefuteLeaf>,
    NiceMock<MockReleaseLeaf>, NiceMock<MockInvalidateQueryLeaf>, NiceMock<MockGetQueryHolders>,
    NiceMock<MockCollectChange>>;

struct PudExpanderTest : public ::testing::Test {
    NiceMock<MockResolve> resolver;
    NiceMock<MockUnfold> unfolder;
    NiceMock<MockActivateChild> activator;
    NiceMock<MockRefuteLeaf> refuter;
    NiceMock<MockReleaseLeaf> releaser;
    NiceMock<MockInvalidateQueryLeaf> queries;
    NiceMock<MockGetQueryHolders> holders;
    NiceMock<MockCollectChange> collector;
    test_expander_t expander{resolver, unfolder, activator, refuter, releaser, queries, holders, collector};
    pud_child_delta first{.substitutions = {}, .body_goals = {}, .var_count = 1};
    pud_child_delta second{.substitutions = {}, .body_goals = {}, .var_count = 2};
    std::vector<pud_child_delta> two_deltas{first, second};

    void SetUp() override {
        ON_CALL(resolver, resolve(4, 1)).WillByDefault(Return(two_deltas));
        ON_CALL(unfolder, unfold_child(4, 1, first)).WillByDefault(Return(10));
        ON_CALL(unfolder, unfold_child(4, 1, second)).WillByDefault(Return(11));
        ON_CALL(queries, invalidate_leaf(_)).WillByDefault(Return(std::vector<pud_mhq_head_id>{}));
        ON_CALL(collector, finish()).WillByDefault(Return(pud_frontier_change{}));
    }
};

}

TEST_F(PudExpanderTest, ChildrenAreActivatedBeforeTheLeafIsReleased) {
    InSequence in_order;
    EXPECT_CALL(activator, activate_child(4, 10));
    EXPECT_CALL(activator, activate_child(4, 11));
    EXPECT_CALL(releaser, release(4));
    EXPECT_CALL(queries, invalidate_leaf(4u));
    expander.expand(4, 1);
}

TEST_F(PudExpanderTest, ChildrenRefutedOnArrivalAreRefutedBeforeTheLeafIsReleased) {
    ON_CALL(activator, activate_child(4, 11)).WillByDefault(Return(true));
    InSequence in_order;
    EXPECT_CALL(refuter, refute(11));
    EXPECT_CALL(releaser, release(4));
    EXPECT_CALL(releaser, release(11));
    EXPECT_CALL(queries, invalidate_leaf(4u));
    EXPECT_CALL(queries, invalidate_leaf(11u));
    expander.expand(4, 1);
}

TEST_F(PudExpanderTest, TheChangeCoversTheChildrenAndEveryRefutedLeaf) {
    const pud_frontier_change change{.offspring = {10}, .refuted = {11, 20}};
    ON_CALL(activator, activate_child(4, 11)).WillByDefault(Return(true));
    ON_CALL(holders, get_holders(_)).WillByDefault(Return(std::vector<pud_node_id>{20}));
    ON_CALL(queries, invalidate_leaf(11u)).WillByDefault(Return(std::vector<pud_mhq_head_id>{5}));
    EXPECT_CALL(collector, add_produced(10));
    EXPECT_CALL(collector, add_produced(11));
    EXPECT_CALL(collector, add_refuted(11));
    EXPECT_CALL(collector, add_refuted(20));
    EXPECT_CALL(collector, finish()).WillOnce(Return(change));
    EXPECT_EQ(expander.expand(4, 1), change);
}
