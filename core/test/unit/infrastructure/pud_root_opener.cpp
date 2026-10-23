#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_root_opener.hpp"
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeRoot {
    MOCK_METHOD(pud_node_id, make_root, (const rule&));
};
struct MockStoreNodeAxiomGroup {
    MOCK_METHOD(void, store_node_group, (pud_node_id, pud_axiom_group_id));
};
struct MockAddAxiomGroupMember {
    MOCK_METHOD(void, add_member, (pud_axiom_group_id, pud_node_id));
};
struct MockActivateRoot {
    MOCK_METHOD(bool, activate_root, (pud_node_id));
};
struct MockRefuteLeaf {
    MOCK_METHOD(void, refute, (pud_node_id));
};
struct MockDeactivateNode {
    MOCK_METHOD(std::vector<pud_mhq_head_id>, deactivate, (pud_node_id));
};
struct MockGetQueryHolders {
    MOCK_METHOD(std::vector<pud_node_id>, get_holders, (pud_mhq_head_id));
};
struct MockCollectChange {
    MOCK_METHOD(void, add_produced, (pud_node_id));
    MOCK_METHOD(void, add_refuted, (pud_node_id));
    MOCK_METHOD(pud_frontier_change, finish, ());
};

using test_root_opener_t = pud_root_opener<
    NiceMock<MockMakeRoot>, NiceMock<MockStoreNodeAxiomGroup>, NiceMock<MockAddAxiomGroupMember>,
    NiceMock<MockActivateRoot>, NiceMock<MockRefuteLeaf>, NiceMock<MockDeactivateNode>, NiceMock<MockGetQueryHolders>,
    NiceMock<MockCollectChange>>;

struct PudRootOpenerTest : public ::testing::Test {
    NiceMock<MockMakeRoot> node_maker;
    NiceMock<MockStoreNodeAxiomGroup> node_groups;
    NiceMock<MockAddAxiomGroupMember> group_members;
    NiceMock<MockActivateRoot> activator;
    NiceMock<MockRefuteLeaf> refuter;
    NiceMock<MockDeactivateNode> deactivator;
    NiceMock<MockGetQueryHolders> holders;
    NiceMock<MockCollectChange> collector;
    test_root_opener_t opener{node_maker, node_groups, group_members, activator, refuter, deactivator, holders, collector};
    pud_axiom_group_id group{.value = 2};
    expr head{expr::functor{1, {}}};
    expr goal{expr::functor{2, {}}};

    void SetUp() override {
        ON_CALL(deactivator, deactivate(_)).WillByDefault(Return(std::vector<pud_mhq_head_id>{}));
        ON_CALL(collector, finish()).WillByDefault(Return(pud_frontier_change{}));
    }
};

}

TEST_F(PudRootOpenerTest, AnAxiomIsMadeRegisteredButNotYetActivated) {
    EXPECT_CALL(node_maker, make_root(rule(&head, {&goal}, 3))).WillOnce(Return(7));
    EXPECT_CALL(node_groups, store_node_group(7u, group));
    EXPECT_CALL(group_members, add_member(group, 7u));
    EXPECT_CALL(activator, activate_root(_)).Times(0);
    EXPECT_EQ(opener.add_axiom(rule(&head, {&goal}, 3), group), 7u);
}

TEST_F(PudRootOpenerTest, OpeningActivatesEveryAddedRootThenRefutesAndReleasesTheRefutedOnes) {
    ON_CALL(node_maker, make_root(_)).WillByDefault(Return(7));
    opener.add_axiom(rule(&head, {}, 0), group);
    ON_CALL(node_maker, make_root(_)).WillByDefault(Return(8));
    opener.add_axiom(rule(&head, {}, 0), group);
    InSequence in_order;
    EXPECT_CALL(activator, activate_root(7)).WillOnce(Return(true));
    EXPECT_CALL(activator, activate_root(8)).WillOnce(Return(false));
    EXPECT_CALL(refuter, refute(7));
    EXPECT_CALL(deactivator, deactivate(7u));
    EXPECT_CALL(collector, add_produced(7));
    EXPECT_CALL(collector, add_produced(8));
    EXPECT_CALL(collector, add_refuted(7));
    EXPECT_CALL(collector, finish());
    opener.open();
}

TEST_F(PudRootOpenerTest, ARootIsOpenedOnlyOnce) {
    ON_CALL(node_maker, make_root(_)).WillByDefault(Return(7));
    opener.add_axiom(rule(&head, {}, 0), group);
    opener.open();
    EXPECT_CALL(activator, activate_root(_)).Times(0);
    opener.open();
}
