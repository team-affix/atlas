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
using ::testing::Expectation;
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
struct MockCheckNodeRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_global_node_id));
};
struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id));
};
struct MockIndexAxiom {
    MOCK_METHOD(void, index_axiom, (pud_global_node_id));
};
struct MockIndexInference {
    MOCK_METHOD(void, index_inference, (pud_global_node_id));
};
struct MockActivateRoot {
    MOCK_METHOD(bool, activate_root, (pud_global_node_id));
};
struct MockStoreLiveChildCount {
    MOCK_METHOD(void, store, (pud_global_node_id, size_t));
};
struct MockActivateChild {
    MOCK_METHOD(bool, activate_child, (pud_global_node_id, pud_global_node_id));
};
struct MockRefuteLeaf {
    MOCK_METHOD(std::vector<pud_global_node_id>, refute, (pud_global_node_id));
};
struct MockDeactivateNode {
    MOCK_METHOD(std::vector<pud_global_node_id>, deactivate, (pud_global_node_id));
};

using test_module_activator_t = pud_module_activator<
    NiceMock<MockListAxioms>, NiceMock<MockCheckNodeRefuted>, NiceMock<MockGetChildren>, NiceMock<MockIndexAxiom>,
    NiceMock<MockIndexInference>, NiceMock<MockActivateRoot>, NiceMock<MockStoreLiveChildCount>,
    NiceMock<MockActivateChild>, NiceMock<MockRefuteLeaf>, NiceMock<MockDeactivateNode>>;

// The stored module, in the test module: axiom 0 has children 2 and 3, axiom 1 is a leaf, 2 is a
// leaf, and 3 has the one child 4, a leaf. A refuted node stays refuted; refuting a node reports it.
struct PudModuleActivatorTest : public ::testing::Test {
    NiceMock<MockListAxioms> axioms;
    NiceMock<MockCheckNodeRefuted> refuted;
    NiceMock<MockGetChildren> children;
    NiceMock<MockIndexAxiom> axiom_indexer;
    NiceMock<MockIndexInference> inference_indexer;
    NiceMock<MockActivateRoot> root_activator;
    NiceMock<MockStoreLiveChildCount> live_child_counts;
    NiceMock<MockActivateChild> child_activator;
    NiceMock<MockRefuteLeaf> refuter;
    NiceMock<MockDeactivateNode> deactivator;
    test_module_activator_t activator{
        axioms, refuted, children, axiom_indexer, inference_indexer, root_activator, live_child_counts,
        child_activator, refuter, deactivator};
    std::vector<pud_global_node_id> axiom_list{pud_test_node(0), pud_test_node(1)};
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> child_lists{
        {0, {2, 3}}, {1, {}}, {2, {}}, {3, {4}}, {4, {}}};
    std::set<pud_global_node_id> refuted_nodes;

    void SetUp() override {
        ON_CALL(axioms, list_axioms(k_test_module)).WillByDefault(ReturnRef(axiom_list));
        for (const auto& [node, kids] : child_lists)
            ON_CALL(children, get_children(pud_test_node(node))).WillByDefault(ReturnRef(kids));
        ON_CALL(refuted, check_refuted(_)).WillByDefault(Invoke(this, &PudModuleActivatorTest::check_refuted));
        ON_CALL(refuter, refute(_)).WillByDefault(Invoke(this, &PudModuleActivatorTest::refute));
        ON_CALL(deactivator, deactivate(_)).WillByDefault(Return(std::vector<pud_global_node_id>{}));
    }

    bool check_refuted(pud_global_node_id node) { return refuted_nodes.contains(node); }

    std::vector<pud_global_node_id> refute(pud_global_node_id node) {
        refuted_nodes.insert(node);
        return {node};
    }
};

}

TEST_F(PudModuleActivatorTest, EveryAxiomIsIndexedBeforeAnyIsActivated) {
    Expectation first_indexed = EXPECT_CALL(axiom_indexer, index_axiom(pud_test_node(0)));
    Expectation second_indexed = EXPECT_CALL(axiom_indexer, index_axiom(pud_test_node(1)));
    EXPECT_CALL(root_activator, activate_root(pud_test_node(0))).After(first_indexed, second_indexed);
    EXPECT_CALL(root_activator, activate_root(pud_test_node(1))).After(first_indexed, second_indexed);
    activator.activate_module(k_test_module);
}

TEST_F(PudModuleActivatorTest, AnAxiomAlreadyRefutedIsNeitherIndexedNorActivatedNorDescended) {
    refuted_nodes.insert(pud_test_node(0));
    EXPECT_CALL(axiom_indexer, index_axiom(_)).Times(AnyNumber());
    EXPECT_CALL(root_activator, activate_root(_)).Times(AnyNumber());
    EXPECT_CALL(axiom_indexer, index_axiom(pud_test_node(0))).Times(0);
    EXPECT_CALL(root_activator, activate_root(pud_test_node(0))).Times(0);
    EXPECT_CALL(inference_indexer, index_inference(_)).Times(0);
    EXPECT_THAT(activator.activate_module(k_test_module).offspring, ElementsAre(pud_test_node(1)));
}

TEST_F(PudModuleActivatorTest, AnAxiomRefutedOnActivationIsRefutedAndReported) {
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

TEST_F(PudModuleActivatorTest, AnInteriorNodeHandsItsPlaceToItsChildrenTheWayAnExpansionDoes) {
    InSequence in_order;
    EXPECT_CALL(inference_indexer, index_inference(pud_test_node(2)));
    EXPECT_CALL(inference_indexer, index_inference(pud_test_node(3)));
    EXPECT_CALL(live_child_counts, store(pud_test_node(0), 2u));
    EXPECT_CALL(child_activator, activate_child(pud_test_node(0), pud_test_node(2)));
    EXPECT_CALL(child_activator, activate_child(pud_test_node(0), pud_test_node(3)));
    EXPECT_CALL(deactivator, deactivate(pud_test_node(0)));
    EXPECT_CALL(inference_indexer, index_inference(pud_test_node(4)));
    EXPECT_CALL(live_child_counts, store(pud_test_node(3), 1u));
    EXPECT_CALL(child_activator, activate_child(pud_test_node(3), pud_test_node(4)));
    EXPECT_CALL(deactivator, deactivate(pud_test_node(3)));
    activator.activate_module(k_test_module);
}

TEST_F(PudModuleActivatorTest, ALeafIsNeverDeactivated) {
    EXPECT_CALL(deactivator, deactivate(_)).Times(AnyNumber());
    EXPECT_CALL(deactivator, deactivate(pud_test_node(1))).Times(0);
    EXPECT_CALL(deactivator, deactivate(pud_test_node(2))).Times(0);
    EXPECT_CALL(deactivator, deactivate(pud_test_node(4))).Times(0);
    activator.activate_module(k_test_module);
}

TEST_F(PudModuleActivatorTest, AChildAlreadyRefutedIsSkippedWithEverythingBelowIt) {
    refuted_nodes.insert(pud_test_node(3));
    EXPECT_CALL(inference_indexer, index_inference(_)).Times(AnyNumber());
    EXPECT_CALL(child_activator, activate_child(_, _)).Times(AnyNumber());
    EXPECT_CALL(live_child_counts, store(_, _)).Times(AnyNumber());
    EXPECT_CALL(inference_indexer, index_inference(pud_test_node(3))).Times(0);
    EXPECT_CALL(inference_indexer, index_inference(pud_test_node(4))).Times(0);
    EXPECT_CALL(child_activator, activate_child(_, pud_test_node(3))).Times(0);
    EXPECT_CALL(live_child_counts, store(pud_test_node(0), 1u));
    EXPECT_THAT(activator.activate_module(k_test_module).offspring, UnorderedElementsAre(pud_test_node(1), pud_test_node(2)));
}

TEST_F(PudModuleActivatorTest, AChildRefutedOnArrivalIsRefutedBeforeTheNextChildIsActivated) {
    ON_CALL(child_activator, activate_child(pud_test_node(0), pud_test_node(2))).WillByDefault(Return(true));
    EXPECT_CALL(child_activator, activate_child(_, _)).Times(AnyNumber());
    EXPECT_CALL(refuter, refute(_)).Times(AnyNumber());
    EXPECT_CALL(deactivator, deactivate(_)).Times(AnyNumber());
    InSequence in_order;
    EXPECT_CALL(child_activator, activate_child(pud_test_node(0), pud_test_node(2)));
    EXPECT_CALL(refuter, refute(pud_test_node(2)));
    EXPECT_CALL(child_activator, activate_child(pud_test_node(0), pud_test_node(3)));
    EXPECT_CALL(deactivator, deactivate(pud_test_node(0)));
    const pud_frontier_change change = activator.activate_module(k_test_module);
    EXPECT_THAT(change.refuted, ElementsAre(pud_test_node(2)));
}

TEST_F(PudModuleActivatorTest, EveryLeafADeactivationStrandsIsRefuted) {
    ON_CALL(deactivator, deactivate(pud_test_node(0)))
        .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(1)}));
    const pud_frontier_change change = activator.activate_module(k_test_module);
    EXPECT_THAT(change.refuted, ElementsAre(pud_test_node(1)));
    EXPECT_THAT(change.offspring, UnorderedElementsAre(pud_test_node(2), pud_test_node(4)));
}

TEST_F(PudModuleActivatorTest, ALeafReachedEarlyButRefutedLaterIsNotOffspring) {
    ON_CALL(deactivator, deactivate(pud_test_node(3)))
        .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(2)}));
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
    EXPECT_CALL(child_activator, activate_child(
        pud_global_node_id{.module = other, .node = 0}, pud_global_node_id{.module = other, .node = 1}));
    EXPECT_THAT(activator.activate_module(other).offspring, ElementsAre(pud_global_node_id{.module = other, .node = 1}));
}

TEST_F(PudModuleActivatorTest, AModuleWithoutAxiomsPutsNothingOnTheFrontier) {
    const std::vector<pud_global_node_id> none{};
    ON_CALL(axioms, list_axioms(k_test_module)).WillByDefault(ReturnRef(none));
    EXPECT_EQ(activator.activate_module(k_test_module), (pud_frontier_change{.offspring = {}, .refuted = {}}));
}
