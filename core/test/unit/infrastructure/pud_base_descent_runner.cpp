#include <map>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_base_descent_runner.hpp"
#include "value_objects/body_goal_id.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Field;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct DescentOps {
    MOCK_METHOD(const std::vector<body_goal_id>&, refuted_goals, (int));
    MOCK_METHOD(int, descend, (int, pud_node_id, pud_node_id));
};

DescentOps* descent_ops = nullptr;

// A descent is a tag; descending from tag t into child c gives tag t * 100 + c.
struct MockDescent {
    struct pq_view {
        int tag;
        const std::vector<body_goal_id>& refuted_goals() const { return descent_ops->refuted_goals(tag); }
    };
    int tag;
    pq_view body_goal_pq() const { return {tag}; }
    MockDescent descend(pud_node_id parent, pud_node_id child) const {
        return {descent_ops->descend(tag, parent, child)};
    }
};

struct MockGetAxioms {
    MOCK_METHOD(const std::vector<pud_node_id>&, get, ());
};
struct MockCheckNodeRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_node_id));
};
struct MockMakeRootDescent {
    MOCK_METHOD(MockDescent, make, (pud_node_id));
};
struct MockCheckNodeLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_node_id));
};
struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get, (pud_node_id));
};
struct MockActivateLeaf {
    MOCK_METHOD(void, activate, (pud_node_id, MockDescent));
};
struct MockRefuteNode {
    MOCK_METHOD(void, refute, (pud_node_id));
};

using test_runner_t = pud_base_descent_runner<
    MockDescent, NiceMock<MockGetAxioms>, NiceMock<MockCheckNodeRefuted>, NiceMock<MockMakeRootDescent>,
    NiceMock<MockCheckNodeLeaf>, NiceMock<MockGetChildren>, NiceMock<MockActivateLeaf>,
    NiceMock<MockRefuteNode>>;

// Axioms 1 and 2. Axiom 1 is an interior node with children 10 (interior, child 100) and 11 (leaf);
// axiom 2 is itself a leaf. Root descents are tagged with the axiom id.
struct PudBaseDescentRunnerTest : public ::testing::Test {
    NiceMock<DescentOps> ops;
    NiceMock<MockGetAxioms> axioms;
    NiceMock<MockCheckNodeRefuted> refuted;
    NiceMock<MockMakeRootDescent> root_descents;
    NiceMock<MockCheckNodeLeaf> leaves;
    NiceMock<MockGetChildren> children;
    NiceMock<MockActivateLeaf> activator;
    NiceMock<MockRefuteNode> refuter;
    test_runner_t runner{axioms, refuted, root_descents, leaves, children, activator, refuter};
    std::vector<pud_node_id> axiom_ids{1, 2};
    std::map<pud_node_id, std::vector<pud_node_id>> children_of{{1, {10, 11}}, {10, {100}}};
    std::vector<pud_node_id> no_children;
    std::vector<body_goal_id> no_refuted_goals;
    std::vector<body_goal_id> one_refuted_goal{0};

    void SetUp() override {
        descent_ops = &ops;
        ON_CALL(axioms, get()).WillByDefault(ReturnRef(axiom_ids));
        ON_CALL(root_descents, make(_)).WillByDefault([](pud_node_id root) {
            return MockDescent{static_cast<int>(root)};
        });
        ON_CALL(leaves, check_leaf(_)).WillByDefault([this](pud_node_id node) {
            return !children_of.contains(node);
        });
        ON_CALL(children, get(_)).WillByDefault([this](pud_node_id node) -> const std::vector<pud_node_id>& {
            const auto found = children_of.find(node);
            return found == children_of.end() ? no_children : found->second;
        });
        ON_CALL(ops, refuted_goals(_)).WillByDefault(ReturnRef(no_refuted_goals));
        ON_CALL(ops, descend(_, _, _)).WillByDefault([](int tag, pud_node_id, pud_node_id child) {
            return static_cast<int>(tag * 100 + child);
        });
    }

    void TearDown() override { descent_ops = nullptr; }
};

}  // namespace

TEST_F(PudBaseDescentRunnerTest, EveryLiveLeafIsActivatedWithTheDescentThatReachedIt) {
    EXPECT_CALL(activator, activate(100, Field(&MockDescent::tag, (1 * 100 + 10) * 100 + 100)));
    EXPECT_CALL(activator, activate(11, Field(&MockDescent::tag, 1 * 100 + 11)));
    EXPECT_CALL(activator, activate(2, Field(&MockDescent::tag, 2)));
    runner.run();
}

TEST_F(PudBaseDescentRunnerTest, InteriorNodesAreNeverActivated) {
    EXPECT_CALL(activator, activate(_, _)).Times(AnyNumber());
    EXPECT_CALL(activator, activate(1, _)).Times(0);
    EXPECT_CALL(activator, activate(10, _)).Times(0);
    runner.run();
}

TEST_F(PudBaseDescentRunnerTest, EachRootDescentIsMadeOnce) {
    EXPECT_CALL(root_descents, make(1)).WillOnce(Return(MockDescent{1}));
    EXPECT_CALL(root_descents, make(2)).WillOnce(Return(MockDescent{2}));
    runner.run();
}

TEST_F(PudBaseDescentRunnerTest, ChildrenAreDescendedFromTheirOwnParent) {
    EXPECT_CALL(ops, descend(1, 1, 10)).WillOnce(Return(110));
    EXPECT_CALL(ops, descend(1, 1, 11)).WillOnce(Return(111));
    EXPECT_CALL(ops, descend(110, 10, 100)).WillOnce(Return(11100));
    runner.run();
}

TEST_F(PudBaseDescentRunnerTest, AxiomsAreRunOneAfterAnother) {
    InSequence in_order;
    EXPECT_CALL(root_descents, make(1)).WillOnce(Return(MockDescent{1}));
    EXPECT_CALL(activator, activate(_, _)).Times(2);
    EXPECT_CALL(root_descents, make(2)).WillOnce(Return(MockDescent{2}));
    EXPECT_CALL(activator, activate(2, _));
    runner.run();
}

TEST_F(PudBaseDescentRunnerTest, RefutedAxiomIsSkippedEntirely) {
    EXPECT_CALL(activator, activate(_, _)).Times(AnyNumber());
    ON_CALL(refuted, check_refuted(1)).WillByDefault(Return(true));
    EXPECT_CALL(root_descents, make(_)).Times(AnyNumber());
    EXPECT_CALL(root_descents, make(1)).Times(0);
    EXPECT_CALL(activator, activate(100, _)).Times(0);
    EXPECT_CALL(activator, activate(11, _)).Times(0);
    EXPECT_CALL(activator, activate(2, _));
    runner.run();
}

TEST_F(PudBaseDescentRunnerTest, RefutedNodeIsSkippedWithItsSubtree) {
    EXPECT_CALL(activator, activate(_, _)).Times(AnyNumber());
    ON_CALL(refuted, check_refuted(10)).WillByDefault(Return(true));
    EXPECT_CALL(activator, activate(100, _)).Times(0);
    EXPECT_CALL(activator, activate(11, _));
    runner.run();
}

TEST_F(PudBaseDescentRunnerTest, NodeRefutedByAnEarlierActivationIsSkipped) {
    EXPECT_CALL(activator, activate(_, _)).Times(AnyNumber());
    bool eleven_activated = false;
    ON_CALL(activator, activate(11, _)).WillByDefault([&](pud_node_id, MockDescent) { eleven_activated = true; });
    ON_CALL(refuted, check_refuted(2)).WillByDefault([&](pud_node_id) { return eleven_activated; });
    EXPECT_CALL(activator, activate(2, _)).Times(0);
    runner.run();
}

TEST_F(PudBaseDescentRunnerTest, InteriorNodeWithARefutedGoalIsRefutedAndNotDescended) {
    EXPECT_CALL(activator, activate(_, _)).Times(AnyNumber());
    EXPECT_CALL(ops, descend(_, _, _)).Times(AnyNumber());
    ON_CALL(ops, refuted_goals(110)).WillByDefault(ReturnRef(one_refuted_goal));
    EXPECT_CALL(refuter, refute(10));
    EXPECT_CALL(ops, descend(110, _, _)).Times(0);
    EXPECT_CALL(activator, activate(100, _)).Times(0);
    EXPECT_CALL(activator, activate(11, _));
    runner.run();
}

TEST_F(PudBaseDescentRunnerTest, LeafWithARefutedGoalIsLeftToTheActivator) {
    EXPECT_CALL(activator, activate(_, _)).Times(AnyNumber());
    ON_CALL(ops, refuted_goals(2)).WillByDefault(ReturnRef(one_refuted_goal));
    EXPECT_CALL(refuter, refute(_)).Times(0);
    EXPECT_CALL(activator, activate(2, _));
    runner.run();
}

TEST_F(PudBaseDescentRunnerTest, EmptyDatabaseDoesNothing) {
    axiom_ids.clear();
    EXPECT_CALL(root_descents, make(_)).Times(0);
    EXPECT_CALL(activator, activate(_, _)).Times(0);
    runner.run();
}
