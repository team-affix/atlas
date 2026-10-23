#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_axiom_registrar.hpp"
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::InSequence;
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

using test_axiom_registrar_t = pud_axiom_registrar<
    NiceMock<MockMakeRoot>, NiceMock<MockStoreNodeAxiomGroup>, NiceMock<MockAddAxiomGroupMember>,
    NiceMock<MockActivateRoot>, NiceMock<MockRefuteLeaf>, NiceMock<MockDeactivateNode>, NiceMock<MockGetQueryHolders>,
    NiceMock<MockCollectChange>>;

struct PudAxiomRegistrarTest : public ::testing::Test {
    NiceMock<MockMakeRoot> node_maker;
    NiceMock<MockStoreNodeAxiomGroup> node_groups;
    NiceMock<MockAddAxiomGroupMember> group_members;
    NiceMock<MockActivateRoot> activator;
    NiceMock<MockRefuteLeaf> refuter;
    NiceMock<MockDeactivateNode> deactivator;
    NiceMock<MockGetQueryHolders> holders;
    NiceMock<MockCollectChange> collector;
    test_axiom_registrar_t registrar{
        node_maker, node_groups, group_members, activator, refuter, deactivator, holders, collector};
    pud_axiom_group_id group{.value = 2};
    expr head{expr::functor{1, {}}};
    expr goal{expr::functor{2, {}}};

    void SetUp() override {
        ON_CALL(deactivator, deactivate(_)).WillByDefault(Return(std::vector<pud_mhq_head_id>{}));
        ON_CALL(collector, finish()).WillByDefault(Return(pud_frontier_change{}));
    }
};

}

TEST_F(PudAxiomRegistrarTest, RegisterAxiomMakesAndRegistersButDoesNotActivate) {
    EXPECT_CALL(node_maker, make_root(rule(&head, {&goal}, 3))).WillOnce(Return(7));
    EXPECT_CALL(node_groups, store_node_group(7u, group));
    EXPECT_CALL(group_members, add_member(group, 7u));
    EXPECT_CALL(activator, activate_root(_)).Times(0);
    EXPECT_EQ(registrar.register_axiom(rule(&head, {&goal}, 3), group), 7u);
}

TEST_F(PudAxiomRegistrarTest, RegisterAxiomGroupRegistersEveryRuleThenActivatesAndRefutes) {
    const rule axiom(&head, {}, 0);
    EXPECT_CALL(node_maker, make_root(_)).WillOnce(Return(7)).WillOnce(Return(8));
    EXPECT_CALL(node_groups, store_node_group(7u, group));
    EXPECT_CALL(node_groups, store_node_group(8u, group));
    EXPECT_CALL(group_members, add_member(group, 7u));
    EXPECT_CALL(group_members, add_member(group, 8u));
    InSequence in_order;
    EXPECT_CALL(activator, activate_root(7)).WillOnce(Return(true));
    EXPECT_CALL(activator, activate_root(8)).WillOnce(Return(false));
    EXPECT_CALL(refuter, refute(7));
    EXPECT_CALL(deactivator, deactivate(7u));
    EXPECT_CALL(collector, add_produced(7));
    EXPECT_CALL(collector, add_produced(8));
    EXPECT_CALL(collector, add_refuted(7));
    EXPECT_CALL(collector, finish());
    registrar.register_axiom_group(group, {axiom, axiom});
}

TEST_F(PudAxiomRegistrarTest, ActivateAxiomGroupOnlyRunsActivationForSuppliedRoots) {
    ON_CALL(node_maker, make_root(_)).WillByDefault(Return(7));
    registrar.register_axiom(rule(&head, {}, 0), group);
    EXPECT_CALL(activator, activate_root(7)).WillOnce(Return(false));
    EXPECT_CALL(collector, add_produced(7));
    EXPECT_CALL(collector, finish());
    registrar.activate_axiom_group(group, {7});
}

TEST_F(PudAxiomRegistrarTest, ActivateAxiomGroupWithNoRootsDoesNotActivate) {
    EXPECT_CALL(activator, activate_root(_)).Times(0);
    EXPECT_CALL(collector, finish());
    registrar.activate_axiom_group(group, {});
}
