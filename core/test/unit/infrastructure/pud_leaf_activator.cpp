#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <immer/map.hpp>
#include "infrastructure/pud_leaf_activator.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/expr.hpp"

using ::testing::_;
using ::testing::Field;
using ::testing::InSequence;
using ::testing::StrictMock;

namespace {

using body_goal_exprs_t = immer::map<body_goal_id, const expr*>;

// A descent is just its open goals and refuted goals.
struct MockDescent {
    struct expr_view {
        const body_goal_exprs_t* goals;
        const body_goal_exprs_t& body_goal_exprs() const { return *goals; }
    };
    struct pq_view {
        const std::vector<body_goal_id>* refuted;
        const std::vector<body_goal_id>& refuted_goals() const { return *refuted; }
    };
    int tag;
    const body_goal_exprs_t* goals;
    const std::vector<body_goal_id>* refuted;
    expr_view body_goal_expr() const { return {goals}; }
    pq_view body_goal_pq() const { return {refuted}; }
};

struct MockStoreNodeDescent {
    MOCK_METHOD(void, store, (pud_node_id, MockDescent));
};
struct MockMarkLeafOpen {
    MOCK_METHOD(void, mark_leaf_open, (pud_node_id));
};
struct MockRefuteNode {
    MOCK_METHOD(void, refute, (pud_node_id));
};

using test_activator_t = pud_leaf_activator<
    MockDescent, StrictMock<MockStoreNodeDescent>, StrictMock<MockMarkLeafOpen>, StrictMock<MockRefuteNode>>;

const expr* expr_at(uintptr_t address) {
    return reinterpret_cast<const expr*>(address);
}

struct PudLeafActivatorTest : public ::testing::Test {
    StrictMock<MockStoreNodeDescent> descents;
    StrictMock<MockMarkLeafOpen> open_leaves;
    StrictMock<MockRefuteNode> refuter;
    test_activator_t activator{descents, open_leaves, refuter};
    pud_node_id leaf = 7;
    body_goal_exprs_t no_goals;
    body_goal_exprs_t two_goals = body_goal_exprs_t{}.set(0, expr_at(0x10)).set(1, expr_at(0x20));
    std::vector<body_goal_id> no_refuted_goals;
    std::vector<body_goal_id> one_refuted_goal{1};
};

}  // namespace

TEST_F(PudLeafActivatorTest, LeafWithOpenGoalsIsStoredThenOpened) {
    InSequence in_order;
    EXPECT_CALL(descents, store(leaf, Field(&MockDescent::tag, 3)));
    EXPECT_CALL(open_leaves, mark_leaf_open(leaf));
    activator.activate(leaf, MockDescent{3, &two_goals, &no_refuted_goals});
}

TEST_F(PudLeafActivatorTest, LeafWithoutGoalsIsStoredButNotOpened) {
    EXPECT_CALL(descents, store(leaf, _));
    activator.activate(leaf, MockDescent{3, &no_goals, &no_refuted_goals});
}

TEST_F(PudLeafActivatorTest, LeafWithARefutedGoalIsStoredThenRefutedAndNeverOpened) {
    InSequence in_order;
    EXPECT_CALL(descents, store(leaf, _));
    EXPECT_CALL(refuter, refute(leaf));
    activator.activate(leaf, MockDescent{3, &two_goals, &one_refuted_goal});
}
