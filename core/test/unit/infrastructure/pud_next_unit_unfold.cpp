#include <deque>
#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_next_unit_unfold.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;

namespace {
struct DescentOps {
    MOCK_METHOD(std::optional<pud_body_goal_id>, find_goal, (pud_node_id, pud_mhq_head_id));
};

DescentOps* descent_ops = nullptr;

struct MockDescent {
    struct pq_view {
        pud_node_id leaf;
        std::optional<pud_body_goal_id> find_goal(pud_mhq_head_id head_id) const {
            return descent_ops->find_goal(leaf, head_id);
        }
    };
    pud_node_id leaf;
    pq_view body_goal_pq() const { return {leaf}; }
};

struct MockCheckUnitQueriesEmpty {
    MOCK_METHOD(bool, empty, ());
};
struct MockFrontUnitQuery {
    MOCK_METHOD(pud_mhq_head_id, front, ());
};
struct MockPopUnitQuery {
    MOCK_METHOD(void, pop, ());
};
struct MockCheckQueryHead {
    MOCK_METHOD(bool, check_head, (pud_mhq_head_id));
};
struct MockGetQueryHeadNode {
    MOCK_METHOD(pud_node_id, get, (pud_mhq_head_id));
};
struct MockCheckNodeRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_node_id));
};
struct MockCheckQueryForked {
    MOCK_METHOD(bool, check_query_forked, (pud_node_id, pud_mhq_head_id));
};
struct MockCheckNodeLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_node_id));
};
struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get, (pud_node_id));
};
struct MockGetNodeDescent {
    MOCK_METHOD(MockDescent, get, (pud_node_id));
};

using test_next_unit_unfold_t = pud_next_unit_unfold<
    NiceMock<MockCheckUnitQueriesEmpty>, NiceMock<MockFrontUnitQuery>, NiceMock<MockPopUnitQuery>,
    NiceMock<MockCheckQueryHead>, NiceMock<MockGetQueryHeadNode>, NiceMock<MockCheckNodeRefuted>,
    NiceMock<MockCheckQueryForked>, NiceMock<MockCheckNodeLeaf>, NiceMock<MockGetChildren>,
    NiceMock<MockGetNodeDescent>>;

struct PudNextUnitUnfoldTest : public ::testing::Test {
    NiceMock<DescentOps> ops;
    NiceMock<MockCheckUnitQueriesEmpty> empty_check;
    NiceMock<MockFrontUnitQuery> front;
    NiceMock<MockPopUnitQuery> pop;
    NiceMock<MockCheckQueryHead> heads;
    NiceMock<MockGetQueryHeadNode> head_nodes;
    NiceMock<MockCheckNodeRefuted> refuted;
    NiceMock<MockCheckQueryForked> forks;
    NiceMock<MockCheckNodeLeaf> leaves;
    NiceMock<MockGetChildren> children;
    NiceMock<MockGetNodeDescent> descents;
    test_next_unit_unfold_t next{empty_check, front, pop, heads, head_nodes, refuted, forks, leaves, children, descents};
    std::deque<pud_mhq_head_id> queue;
    std::map<pud_node_id, std::vector<pud_node_id>> children_of{{1, {2, 3}}, {2, {10, 11}}, {3, {12}}};
    std::vector<pud_node_id> no_children;
    std::map<std::pair<pud_node_id, pud_mhq_head_id>, pud_body_goal_id> held{{{11, 40}, 2}, {{12, 50}, 0}};

    void SetUp() override {
        descent_ops = &ops;
        ON_CALL(empty_check, empty()).WillByDefault([this] { return queue.empty(); });
        ON_CALL(front, front()).WillByDefault([this] { return queue.front(); });
        ON_CALL(pop, pop()).WillByDefault([this] { queue.pop_front(); });
        ON_CALL(heads, check_head(_)).WillByDefault(Return(true));
        ON_CALL(head_nodes, get(_)).WillByDefault(Return(1));
        ON_CALL(leaves, check_leaf(_)).WillByDefault([this](pud_node_id node) { return !children_of.contains(node); });
        ON_CALL(children, get(_)).WillByDefault([this](pud_node_id node) -> const std::vector<pud_node_id>& {
            const auto found = children_of.find(node);
            return found == children_of.end() ? no_children : found->second;
        });
        ON_CALL(descents, get(_)).WillByDefault([](pud_node_id leaf) { return MockDescent{leaf}; });
        ON_CALL(ops, find_goal(_, _)).WillByDefault([this](pud_node_id leaf, pud_mhq_head_id head_id) {
            const auto found = held.find({leaf, head_id});
            return found == held.end() ? std::optional<pud_body_goal_id>{} : std::optional<pud_body_goal_id>{found->second};
        });
    }

    void TearDown() override { descent_ops = nullptr; }
};

}

TEST_F(PudNextUnitUnfoldTest, NothingIsForcedWhenNoQueryIsUnit) {
    EXPECT_FALSE(next.get_next_unit_unfold().has_value());
}

TEST_F(PudNextUnitUnfoldTest, LeafHoldingTheUnitHeadIsUnfoldedOverThatGoal) {
    queue = {40};
    EXPECT_EQ(next.get_next_unit_unfold(), (pud_unfold{11, 2}));
}

TEST_F(PudNextUnitUnfoldTest, HeadStaysQueuedWhileALeafStillHoldsIt) {
    queue = {40};
    next.get_next_unit_unfold();
    EXPECT_EQ(queue, (std::deque<pud_mhq_head_id>{40}));
}

TEST_F(PudNextUnitUnfoldTest, OldestUnitQueryIsServedFirst) {
    queue = {50, 40};
    EXPECT_EQ(next.get_next_unit_unfold(), (pud_unfold{12, 0}));
}

TEST_F(PudNextUnitUnfoldTest, SearchStartsAtTheNodeThatOpenedTheQuery) {
    queue = {40};
    ON_CALL(head_nodes, get(40u)).WillByDefault(Return(2));
    EXPECT_CALL(children, get(_)).Times(AnyNumber());
    EXPECT_CALL(children, get(1u)).Times(0);
    EXPECT_EQ(next.get_next_unit_unfold(), (pud_unfold{11, 2}));
}

TEST_F(PudNextUnitUnfoldTest, GoneHeadIsPoppedWithoutSearching) {
    queue = {40, 50};
    ON_CALL(heads, check_head(40u)).WillByDefault(Return(false));
    EXPECT_CALL(head_nodes, get(_)).Times(AnyNumber());
    EXPECT_CALL(head_nodes, get(40u)).Times(0);
    EXPECT_EQ(next.get_next_unit_unfold(), (pud_unfold{12, 0}));
    EXPECT_EQ(queue, (std::deque<pud_mhq_head_id>{50}));
}

TEST_F(PudNextUnitUnfoldTest, HeadNoLeafHoldsAnyMoreIsPopped) {
    queue = {40, 50};
    held.erase({11, 40});
    EXPECT_EQ(next.get_next_unit_unfold(), (pud_unfold{12, 0}));
    EXPECT_EQ(queue, (std::deque<pud_mhq_head_id>{50}));
}

TEST_F(PudNextUnitUnfoldTest, SubtreeWhereTheQueryWasForkedIsSkipped) {
    queue = {40};
    held[{12, 40}] = 5;
    ON_CALL(forks, check_query_forked(2, 40u)).WillByDefault(Return(true));
    EXPECT_CALL(children, get(_)).Times(AnyNumber());
    EXPECT_CALL(children, get(2u)).Times(0);
    EXPECT_EQ(next.get_next_unit_unfold(), (pud_unfold{12, 5}));
}

TEST_F(PudNextUnitUnfoldTest, ForkOfAnotherHeadAtANodeDoesNotSkipIt) {
    queue = {40};
    ON_CALL(forks, check_query_forked(2, 41u)).WillByDefault(Return(true));
    EXPECT_EQ(next.get_next_unit_unfold(), (pud_unfold{11, 2}));
}

TEST_F(PudNextUnitUnfoldTest, ForkedLeafIsSkipped) {
    queue = {40};
    ON_CALL(forks, check_query_forked(11, 40u)).WillByDefault(Return(true));
    EXPECT_CALL(ops, find_goal(_, _)).Times(AnyNumber());
    EXPECT_CALL(ops, find_goal(11u, _)).Times(0);
    EXPECT_FALSE(next.get_next_unit_unfold().has_value());
}

TEST_F(PudNextUnitUnfoldTest, RefutedSubtreeIsSkipped) {
    queue = {40};
    ON_CALL(refuted, check_refuted(2)).WillByDefault(Return(true));
    EXPECT_CALL(children, get(_)).Times(AnyNumber());
    EXPECT_CALL(children, get(2u)).Times(0);
    EXPECT_FALSE(next.get_next_unit_unfold().has_value());
    EXPECT_TRUE(queue.empty());
}

TEST_F(PudNextUnitUnfoldTest, LeafHoldingOnlyAForkOfTheHeadIsPassedOver) {
    queue = {40};
    held = {{{10, 41}, 2}, {{11, 40}, 2}};
    EXPECT_EQ(next.get_next_unit_unfold(), (pud_unfold{11, 2}));
}

TEST_F(PudNextUnitUnfoldTest, FirstHoldingLeafFoundIsReturnedAlone) {
    queue = {40};
    held[{10, 40}] = 3;
    const std::optional<pud_unfold> found = next.get_next_unit_unfold();
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->leaf == 10 || found->leaf == 11);
    EXPECT_EQ(queue, (std::deque<pud_mhq_head_id>{40}));
}

TEST_F(PudNextUnitUnfoldTest, StaleQueueDrainsToNothingForced) {
    queue = {40, 50};
    held.clear();
    EXPECT_FALSE(next.get_next_unit_unfold().has_value());
    EXPECT_TRUE(queue.empty());
}
