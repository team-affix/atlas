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
struct MockRefuteFrontier {
    MOCK_METHOD(std::vector<pud_node_id>, refute, (const std::vector<pud_node_id>&, const std::vector<pud_node_id>&));
};
struct MockCollectChange {
    MOCK_METHOD(pud_frontier_change, collect, (const std::vector<pud_node_id>&, const std::vector<pud_node_id>&));
};

using test_expander_t = pud_expander<
    NiceMock<MockResolve>, NiceMock<MockUnfold>, NiceMock<MockActivateChild>, NiceMock<MockRefuteFrontier>,
    NiceMock<MockCollectChange>>;

struct PudExpanderTest : public ::testing::Test {
    NiceMock<MockResolve> resolver;
    NiceMock<MockUnfold> unfolder;
    NiceMock<MockActivateChild> activator;
    NiceMock<MockRefuteFrontier> refuter;
    NiceMock<MockCollectChange> collector;
    test_expander_t expander{resolver, unfolder, activator, refuter, collector};
    std::vector<pud_child_delta> deltas{pud_child_delta{.substitutions = {}, .body_goals = {}, .var_count = 1}};

    void SetUp() override {
        ON_CALL(resolver, resolve(4, 1)).WillByDefault(Return(deltas));
        ON_CALL(unfolder, unfold(4, 1, deltas)).WillByDefault(Return(std::vector<pud_node_id>{10, 11}));
    }
};

}

TEST_F(PudExpanderTest, ChildrenAreActivatedBeforeTheLeafIsRefuted) {
    InSequence in_order;
    EXPECT_CALL(activator, activate_child(4, 10));
    EXPECT_CALL(activator, activate_child(4, 11));
    EXPECT_CALL(refuter, refute(ElementsAre(4u), IsEmpty()));
    expander.expand(4, 1);
}

TEST_F(PudExpanderTest, ChildrenRefutedOnArrivalAreRefutedWithTheLeaf) {
    ON_CALL(activator, activate_child(4, 11)).WillByDefault(Return(true));
    EXPECT_CALL(refuter, refute(ElementsAre(4u), ElementsAre(11u)));
    expander.expand(4, 1);
}

TEST_F(PudExpanderTest, TheChangeCoversTheChildrenAndEveryRefutedLeaf) {
    const pud_frontier_change change{.offspring = {10}, .refuted = {11, 20}};
    ON_CALL(refuter, refute(_, _)).WillByDefault(Return(std::vector<pud_node_id>{11, 20}));
    EXPECT_CALL(collector, collect(ElementsAre(10u, 11u), ElementsAre(11u, 20u))).WillOnce(Return(change));
    EXPECT_EQ(expander.expand(4, 1), change);
}
