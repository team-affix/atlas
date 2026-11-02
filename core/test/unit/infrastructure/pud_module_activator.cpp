#include <set>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_test_node.hpp"
#include "infrastructure/pud_module_activator.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::UnorderedElementsAre;

namespace {

struct MockListAxioms {
    MOCK_METHOD(const std::vector<pud_global_node_id>&, list_axioms, (pud_module_id));
};
struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id));
};
struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id));
};
struct MockActivateRoot {
    MOCK_METHOD(bool, activate_root, (pud_global_node_id));
};
struct MockRefuteLeaf {
    MOCK_METHOD(std::vector<pud_global_node_id>, refute, (pud_global_node_id));
};
struct MockMoveActivity {
    MOCK_METHOD(std::vector<pud_global_node_id>, move, (pud_global_node_id, (const std::vector<pud_global_node_id>&)));
};

using test_module_activator_t = pud_module_activator<
    NiceMock<MockListAxioms>, NiceMock<MockIsNodeRefuted>, NiceMock<MockGetChildren>, NiceMock<MockActivateRoot>,
    NiceMock<MockRefuteLeaf>, NiceMock<MockMoveActivity>>;

// The stored module, in the test module: axiom 0 has children 2 and 3, axiom 1 is a leaf, 2 is a
// leaf, and 3 has the one child 4, a leaf. A refuted node stays refuted; refuting a node reports it.
// Moving activity off a parent refutes the nodes listed for it in refuted_by_move.
struct PudModuleActivatorTest : public ::testing::Test {
    NiceMock<MockListAxioms> axioms;
    NiceMock<MockIsNodeRefuted> refuted;
    NiceMock<MockGetChildren> children;
    NiceMock<MockActivateRoot> root_activator;
    NiceMock<MockRefuteLeaf> refuter;
    NiceMock<MockMoveActivity> mover;
    test_module_activator_t activator{axioms, refuted, children, root_activator, refuter, mover};
    std::vector<pud_global_node_id> axiom_list{pud_test_node(0), pud_test_node(1)};
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> child_lists{
        {0, {2, 3}}, {1, {}}, {2, {}}, {3, {4}}, {4, {}}};
    std::set<pud_global_node_id> refuted_nodes;
    std::unordered_map<pud_node_id, std::vector<pud_global_node_id>> refuted_by_move;

    void SetUp() override {
        ON_CALL(axioms, list_axioms(k_test_module)).WillByDefault(ReturnRef(axiom_list));
        for (const auto& [node, kids] : child_lists)
            ON_CALL(children, get_children(pud_test_node(node))).WillByDefault(ReturnRef(kids));
        ON_CALL(refuted, is_refuted(_)).WillByDefault(Invoke(this, &PudModuleActivatorTest::is_refuted));
        ON_CALL(refuter, refute(_)).WillByDefault(Invoke(this, &PudModuleActivatorTest::refute));
        ON_CALL(mover, move(_, _)).WillByDefault(Invoke(this, &PudModuleActivatorTest::move));
    }

    bool is_refuted(pud_global_node_id node) { return refuted_nodes.contains(node); }

    std::vector<pud_global_node_id> refute(pud_global_node_id node) {
        refuted_nodes.insert(node);
        return {node};
    }

    std::vector<pud_global_node_id> move(pud_global_node_id parent, const std::vector<pud_global_node_id>&) {
        const std::vector<pud_global_node_id> refuted_now = refuted_by_move[parent.node];
        refuted_nodes.insert(refuted_now.begin(), refuted_now.end());
        return refuted_now;
    }
};

}

TEST_F(PudModuleActivatorTest, EveryAxiomIsActivatedInOrder) {
    InSequence in_order;
    EXPECT_CALL(root_activator, activate_root(pud_test_node(0)));
    EXPECT_CALL(root_activator, activate_root(pud_test_node(1)));
    activator.activate_module(k_test_module);
}

TEST_F(PudModuleActivatorTest, AnAxiomAlreadyRefutedIsNeitherActivatedNorDescended) {
    refuted_nodes.insert(pud_test_node(0));
    EXPECT_CALL(root_activator, activate_root(_)).Times(AnyNumber());
    EXPECT_CALL(root_activator, activate_root(pud_test_node(0))).Times(0);
    EXPECT_CALL(mover, move(_, _)).Times(0);
    EXPECT_THAT(activator.activate_module(k_test_module).offspring, ElementsAre(pud_test_node(1)));
}

TEST_F(PudModuleActivatorTest, AnAxiomRefutedOnArrivalIsRefutedOnlyOnceEveryAxiomIsActivated) {
    ON_CALL(root_activator, activate_root(pud_test_node(0))).WillByDefault(Return(true));
    EXPECT_CALL(refuter, refute(_)).Times(AnyNumber());
    InSequence in_order;
    EXPECT_CALL(root_activator, activate_root(pud_test_node(0)));
    EXPECT_CALL(root_activator, activate_root(pud_test_node(1)));
    EXPECT_CALL(refuter, refute(pud_test_node(0)));
    activator.activate_module(k_test_module);
}

TEST_F(PudModuleActivatorTest, AnAxiomRefutedOnArrivalIsReported) {
    ON_CALL(root_activator, activate_root(pud_test_node(1))).WillByDefault(Return(true));
    const pud_frontier_change change = activator.activate_module(k_test_module);
    EXPECT_THAT(change.refuted, ElementsAre(pud_test_node(1)));
    EXPECT_THAT(change.offspring, UnorderedElementsAre(pud_test_node(2), pud_test_node(4)));
}

TEST_F(PudModuleActivatorTest, TheDescentReachesEveryStoredLeafAndReportsThemAsOffspring) {
    const pud_frontier_change change = activator.activate_module(k_test_module);
    EXPECT_THAT(change.offspring, UnorderedElementsAre(pud_test_node(1), pud_test_node(2), pud_test_node(4)));
    EXPECT_THAT(change.refuted, IsEmpty());
}

TEST_F(PudModuleActivatorTest, EachInteriorNodeMovesActivityToItsChildrenLevelByLevel) {
    InSequence in_order;
    EXPECT_CALL(mover, move(pud_test_node(0), ElementsAre(pud_test_node(2), pud_test_node(3))));
    EXPECT_CALL(mover, move(pud_test_node(3), ElementsAre(pud_test_node(4))));
    activator.activate_module(k_test_module);
}

TEST_F(PudModuleActivatorTest, ALeafNeverMovesActivity) {
    EXPECT_CALL(mover, move(_, _)).Times(AnyNumber());
    EXPECT_CALL(mover, move(pud_test_node(1), _)).Times(0);
    EXPECT_CALL(mover, move(pud_test_node(2), _)).Times(0);
    EXPECT_CALL(mover, move(pud_test_node(4), _)).Times(0);
    activator.activate_module(k_test_module);
}

TEST_F(PudModuleActivatorTest, AChildAlreadyRefutedIsSkippedWithEverythingBelowIt) {
    refuted_nodes.insert(pud_test_node(3));
    EXPECT_CALL(mover, move(pud_test_node(0), ElementsAre(pud_test_node(2))));
    EXPECT_CALL(mover, move(pud_test_node(3), _)).Times(0);
    EXPECT_THAT(activator.activate_module(k_test_module).offspring, UnorderedElementsAre(pud_test_node(1), pud_test_node(2)));
}

TEST_F(PudModuleActivatorTest, AChildAMoveRefutesIsReportedAndNotDescended) {
    refuted_by_move[0] = {pud_test_node(3)};
    EXPECT_CALL(mover, move(_, _)).Times(AnyNumber());
    EXPECT_CALL(mover, move(pud_test_node(3), _)).Times(0);
    const pud_frontier_change change = activator.activate_module(k_test_module);
    EXPECT_THAT(change.refuted, ElementsAre(pud_test_node(3)));
    EXPECT_THAT(change.offspring, UnorderedElementsAre(pud_test_node(1), pud_test_node(2)));
}

TEST_F(PudModuleActivatorTest, ALeafAMoveStrandsIsReportedAndNotOffspring) {
    refuted_by_move[0] = {pud_test_node(1)};
    const pud_frontier_change change = activator.activate_module(k_test_module);
    EXPECT_THAT(change.refuted, ElementsAre(pud_test_node(1)));
    EXPECT_THAT(change.offspring, UnorderedElementsAre(pud_test_node(2), pud_test_node(4)));
}

TEST_F(PudModuleActivatorTest, ALeafReachedEarlyButRefutedLaterIsNotOffspring) {
    refuted_by_move[3] = {pud_test_node(2)};
    const pud_frontier_change change = activator.activate_module(k_test_module);
    EXPECT_THAT(change.offspring, UnorderedElementsAre(pud_test_node(1), pud_test_node(4)));
    EXPECT_THAT(change.refuted, ElementsAre(pud_test_node(2)));
}

TEST_F(PudModuleActivatorTest, ChildrenAreNamedInTheModuleBeingActivated) {
    const pud_module_id other = k_test_module + 1;
    const std::vector<pud_global_node_id> other_axioms{pud_global_node_id{.module = other, .node = 0}};
    const std::vector<pud_node_id> other_children{1};
    const std::vector<pud_node_id> no_children{};
    ON_CALL(axioms, list_axioms(other)).WillByDefault(ReturnRef(other_axioms));
    ON_CALL(children, get_children(pud_global_node_id{.module = other, .node = 0})).WillByDefault(ReturnRef(other_children));
    ON_CALL(children, get_children(pud_global_node_id{.module = other, .node = 1})).WillByDefault(ReturnRef(no_children));
    EXPECT_CALL(mover, move(
        pud_global_node_id{.module = other, .node = 0}, ElementsAre(pud_global_node_id{.module = other, .node = 1})));
    EXPECT_THAT(activator.activate_module(other).offspring, ElementsAre(pud_global_node_id{.module = other, .node = 1}));
}

TEST_F(PudModuleActivatorTest, AModuleWithoutAxiomsPutsNothingOnTheFrontier) {
    const std::vector<pud_global_node_id> none{};
    ON_CALL(axioms, list_axioms(k_test_module)).WillByDefault(ReturnRef(none));
    EXPECT_EQ(activator.activate_module(k_test_module), (pud_frontier_change{.offspring = {}, .refuted = {}}));
}
