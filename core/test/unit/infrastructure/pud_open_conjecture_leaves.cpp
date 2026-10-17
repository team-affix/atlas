#include <set>
#include <unordered_map>
#include <unordered_set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_open_conjecture_leaves.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMarkLeafOpen {
    MOCK_METHOD(void, mark_leaf_open, (pud_node_id));
};
struct MockMarkLeafClosed {
    MOCK_METHOD(void, mark_leaf_closed, (pud_node_id));
};
struct MockCheckNodeRoot {
    MOCK_METHOD(bool, check_root, (pud_node_id));
};
struct MockCheckAxiom {
    MOCK_METHOD(bool, check_axiom, (pud_node_id));
};
struct MockGetNodeParent {
    MOCK_METHOD(pud_node_id, get, (pud_node_id));
};

using test_tracker_t = pud_open_conjecture_leaves<
    NiceMock<MockMarkLeafOpen>, NiceMock<MockMarkLeafClosed>, NiceMock<MockCheckNodeRoot>,
    NiceMock<MockCheckAxiom>, NiceMock<MockGetNodeParent>>;

// Axiom root 1 with child 10; conjecture root 2 with child 20 and grandchild 200.
struct PudOpenConjectureLeavesTest : public ::testing::Test {
    NiceMock<MockMarkLeafOpen> open_leaves;
    NiceMock<MockMarkLeafClosed> close_leaves;
    NiceMock<MockCheckNodeRoot> roots;
    NiceMock<MockCheckAxiom> axioms;
    NiceMock<MockGetNodeParent> parents;
    test_tracker_t tracker{open_leaves, close_leaves, roots, axioms, parents};
    std::unordered_map<pud_node_id, pud_node_id> parent_of{{10, 1}, {20, 2}, {200, 20}};

    void SetUp() override {
        ON_CALL(roots, check_root(_)).WillByDefault([](pud_node_id node) { return node == 1 || node == 2; });
        ON_CALL(axioms, check_axiom(_)).WillByDefault([](pud_node_id node) { return node == 1; });
        ON_CALL(parents, get(_)).WillByDefault([this](pud_node_id node) { return parent_of.at(node); });
    }
};

}  // namespace

TEST_F(PudOpenConjectureLeavesTest, OpeningAlwaysReachesTheWrappedStore) {
    EXPECT_CALL(open_leaves, mark_leaf_open(1));
    EXPECT_CALL(open_leaves, mark_leaf_open(2));
    tracker.mark_leaf_open(1);
    tracker.mark_leaf_open(2);
}

TEST_F(PudOpenConjectureLeavesTest, ClosingAlwaysReachesTheWrappedStore) {
    tracker.mark_leaf_open(1);
    tracker.mark_leaf_open(2);
    EXPECT_CALL(close_leaves, mark_leaf_closed(1));
    EXPECT_CALL(close_leaves, mark_leaf_closed(2));
    tracker.mark_leaf_closed(1);
    tracker.mark_leaf_closed(2);
}

TEST_F(PudOpenConjectureLeavesTest, OpenConjectureRootIsListed) {
    tracker.mark_leaf_open(2);
    EXPECT_EQ(tracker.get(), (std::set<pud_node_id>{2}));
}

TEST_F(PudOpenConjectureLeavesTest, OpenAxiomLeavesAreNeverListed) {
    tracker.mark_leaf_open(1);
    tracker.mark_leaf_closed(1);
    tracker.mark_leaf_open(10);
    EXPECT_THAT(tracker.get(), IsEmpty());
}

TEST_F(PudOpenConjectureLeavesTest, DescendantsOfAConjectureAreListedAtAnyDepth) {
    tracker.mark_leaf_open(2);
    tracker.mark_leaf_closed(2);
    tracker.mark_leaf_open(20);
    tracker.mark_leaf_closed(20);
    tracker.mark_leaf_open(200);
    EXPECT_EQ(tracker.get(), (std::set<pud_node_id>{200}));
}

TEST_F(PudOpenConjectureLeavesTest, ClosedConjectureLeafIsNoLongerListed) {
    tracker.mark_leaf_open(2);
    tracker.mark_leaf_closed(2);
    EXPECT_THAT(tracker.get(), IsEmpty());
}

TEST_F(PudOpenConjectureLeavesTest, ConjectureAndAxiomLeavesOpenTogetherOnlyListTheConjectures) {
    tracker.mark_leaf_open(1);
    tracker.mark_leaf_open(2);
    tracker.mark_leaf_closed(1);
    tracker.mark_leaf_open(10);
    EXPECT_EQ(tracker.get(), (std::set<pud_node_id>{2}));
}

TEST_F(PudOpenConjectureLeavesTest, OpeningForwardsBeforeTrackingAndClosingUntracksBeforeForwarding) {
    InSequence in_order;
    EXPECT_CALL(open_leaves, mark_leaf_open(2));
    EXPECT_CALL(roots, check_root(2));
    tracker.mark_leaf_open(2);
    EXPECT_CALL(close_leaves, mark_leaf_closed(2)).WillOnce([this](pud_node_id) {
        EXPECT_THAT(tracker.get(), IsEmpty());
    });
    tracker.mark_leaf_closed(2);
}
