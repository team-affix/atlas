#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_expander.hpp"
#include "value_objects/pud_child_delta.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockResolve {
    MOCK_METHOD(std::vector<pud_child_delta>, resolve, (pud_node_id, pud_body_goal_id));
};
struct MockUnfold {
    MOCK_METHOD(std::vector<pud_node_id>, unfold, (pud_node_id, pud_body_goal_id, const std::vector<pud_child_delta>&));
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
struct MockInvalidateQueryLeaves {
    MOCK_METHOD(std::vector<pud_mhq_head_id>, invalidate_leaves, ((const std::vector<pud_node_id>&)));
};
struct MockGetQueryHolders {
    MOCK_METHOD(std::vector<pud_node_id>, get_holders, (pud_mhq_head_id));
};
struct MockCollectChange {
    MOCK_METHOD(pud_frontier_change, collect, (const std::vector<pud_node_id>&, const std::vector<pud_node_id>&));
};

using test_expander_t = pud_expander<
    NiceMock<MockResolve>, NiceMock<MockUnfold>, NiceMock<MockActivateChild>, NiceMock<MockRefuteLeaf>,
    NiceMock<MockReleaseLeaf>, NiceMock<MockInvalidateQueryLeaves>, NiceMock<MockGetQueryHolders>,
    NiceMock<MockCollectChange>>;

struct PudExpanderTest : public ::testing::Test {
    NiceMock<MockResolve> resolver;
    NiceMock<MockUnfold> unfolder;
    NiceMock<MockActivateChild> activator;
    NiceMock<MockRefuteLeaf> refuter;
    NiceMock<MockReleaseLeaf> releaser;
    NiceMock<MockInvalidateQueryLeaves> queries;
    NiceMock<MockGetQueryHolders> holders;
    NiceMock<MockCollectChange> collector;
    test_expander_t expander{resolver, unfolder, activator, refuter, releaser, queries, holders, collector};
    std::vector<pud_child_delta> deltas{pud_child_delta{.substitutions = {}, .body_goals = {}, .var_count = 1}};

    void SetUp() override {
        ON_CALL(resolver, resolve(4, 1)).WillByDefault(Return(deltas));
        ON_CALL(unfolder, unfold(4, 1, deltas)).WillByDefault(Return(std::vector<pud_node_id>{10, 11}));
        ON_CALL(queries, invalidate_leaves(_)).WillByDefault(Return(std::vector<pud_mhq_head_id>{}));
    }
};

}

TEST_F(PudExpanderTest, ChildrenAreActivatedBeforeTheLeafIsReleased) {
    InSequence in_order;
    EXPECT_CALL(activator, activate_child(4, 10));
    EXPECT_CALL(activator, activate_child(4, 11));
    EXPECT_CALL(releaser, release(4));
    EXPECT_CALL(queries, invalidate_leaves(ElementsAre(4u)));
    expander.expand(4, 1);
}

TEST_F(PudExpanderTest, ChildrenRefutedOnArrivalAreRefutedBeforeTheLeafIsReleased) {
    ON_CALL(activator, activate_child(4, 11)).WillByDefault(Return(true));
    InSequence in_order;
    EXPECT_CALL(refuter, refute(11));
    EXPECT_CALL(releaser, release(4));
    EXPECT_CALL(releaser, release(11));
    EXPECT_CALL(queries, invalidate_leaves(ElementsAre(4u, 11u)));
    expander.expand(4, 1);
}

TEST_F(PudExpanderTest, TheChangeCoversTheChildrenAndEveryRefutedLeaf) {
    const pud_frontier_change change{.offspring = {10}, .refuted = {11, 20}};
    ON_CALL(activator, activate_child(4, 11)).WillByDefault(Return(true));
    ON_CALL(holders, get_holders(_)).WillByDefault(Return(std::vector<pud_node_id>{20}));
    ON_CALL(queries, invalidate_leaves(ElementsAre(4u, 11u))).WillByDefault(Return(std::vector<pud_mhq_head_id>{5}));
    EXPECT_CALL(collector, collect(ElementsAre(10u, 11u), ElementsAre(11u, 20u))).WillOnce(Return(change));
    EXPECT_EQ(expander.expand(4, 1), change);
}
