#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <immer/map.hpp>
#include "infrastructure/pud_solver.hpp"
#include "value_objects/expr.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

using body_goal_exprs_t = immer::map<body_goal_id, const expr*>;

struct DescentOps {
    MOCK_METHOD(const body_goal_exprs_t&, body_goal_exprs, (int));
    MOCK_METHOD(const std::vector<body_goal_id>&, refuted_goals, (int));
};

DescentOps* descent_ops = nullptr;

struct MockDescent {
    struct expr_view {
        int tag;
        const body_goal_exprs_t& body_goal_exprs() const { return descent_ops->body_goal_exprs(tag); }
    };
    struct pq_view {
        int tag;
        const std::vector<body_goal_id>& refuted_goals() const { return descent_ops->refuted_goals(tag); }
    };
    int tag;
    expr_view body_goal_expr() const { return {tag}; }
    pq_view body_goal_pq() const { return {tag}; }
};

struct MockFindLiveLeaves {
    MOCK_METHOD(std::vector<pud_node_id>, find_live_leaves, (pud_node_id));
};
struct MockGetNodeDescent {
    MOCK_METHOD(const MockDescent&, get, (pud_node_id));
};
struct MockExpandLeaf {
    MOCK_METHOD(void, expand, (pud_node_id, body_goal_id));
};
struct MockMakeRootDescent {
    MOCK_METHOD(MockDescent, make, (pud_node_id));
};
struct MockStoreNodeDescent {
    MOCK_METHOD(void, store, (pud_node_id, MockDescent));
};
struct MockRefuteNode {
    MOCK_METHOD(void, refute, (pud_node_id));
};

using test_solver_t = pud_solver<
    NiceMock<MockFindLiveLeaves>, NiceMock<MockGetNodeDescent>, NiceMock<MockExpandLeaf>,
    NiceMock<MockMakeRootDescent>, NiceMock<MockStoreNodeDescent>, NiceMock<MockRefuteNode>>;

const expr* expr_at(uintptr_t address) {
    return reinterpret_cast<const expr*>(address);
}

struct PudSolverTest : public ::testing::Test {
    NiceMock<DescentOps> ops;
    NiceMock<MockFindLiveLeaves> live_leaves;
    NiceMock<MockGetNodeDescent> descents;
    NiceMock<MockExpandLeaf> expander;
    NiceMock<MockMakeRootDescent> root_maker;
    NiceMock<MockStoreNodeDescent> store_descent;
    NiceMock<MockRefuteNode> refuter;
    test_solver_t solver{live_leaves, descents, expander, root_maker, store_descent, refuter};
    pud_node_id root = 1;
    std::unordered_map<pud_node_id, MockDescent> descent_of;
    std::unordered_map<int, body_goal_exprs_t> goals_of;
    std::vector<body_goal_id> no_refuted_goals;
    std::vector<body_goal_id> one_refuted_goal{0};

    void give_leaf(pud_node_id leaf, body_goal_exprs_t goals) {
        const int tag = static_cast<int>(leaf);
        descent_of[leaf] = MockDescent{tag};
        goals_of[tag] = std::move(goals);
    }

    void SetUp() override {
        descent_ops = &ops;
        ON_CALL(descents, get(_)).WillByDefault([&](pud_node_id node) -> const MockDescent& {
            return descent_of.at(node);
        });
        ON_CALL(ops, body_goal_exprs(_)).WillByDefault([&](int tag) -> const body_goal_exprs_t& {
            return goals_of.at(tag);
        });
        ON_CALL(ops, refuted_goals(_)).WillByDefault(ReturnRef(no_refuted_goals));
        ON_CALL(root_maker, make(_)).WillByDefault(Return(MockDescent{0}));
    }

    void TearDown() override { descent_ops = nullptr; }
};

// ---------- start ----------

TEST_F(PudSolverTest, StartStoresTheRootsDescent) {
    EXPECT_CALL(root_maker, make(root)).WillOnce(Return(MockDescent{7}));
    EXPECT_CALL(store_descent, store(root, _));
    solver.start(root);
}

TEST_F(PudSolverTest, StartRefutesARootArrivingWithARefutedGoal) {
    EXPECT_CALL(root_maker, make(root)).WillOnce(Return(MockDescent{7}));
    ON_CALL(ops, refuted_goals(7)).WillByDefault(ReturnRef(one_refuted_goal));
    InSequence in_order;
    EXPECT_CALL(store_descent, store(root, _));
    EXPECT_CALL(refuter, refute(root));
    solver.start(root);
}

TEST_F(PudSolverTest, StartLeavesALiveRootAlone) {
    EXPECT_CALL(refuter, refute(_)).Times(0);
    solver.start(root);
}

// ---------- solve ----------

TEST_F(PudSolverTest, RefutedTreeHasNoSolution) {
    EXPECT_CALL(live_leaves, find_live_leaves(root)).WillOnce(Return(std::vector<pud_node_id>{}));
    EXPECT_FALSE(solver.solve(root, 10).has_value());
}

TEST_F(PudSolverTest, LeafWithNoOpenGoalsIsASolution) {
    give_leaf(4, {});
    EXPECT_CALL(live_leaves, find_live_leaves(root)).WillOnce(Return(std::vector<pud_node_id>{4}));
    EXPECT_CALL(expander, expand(_, _)).Times(0);
    EXPECT_EQ(solver.solve(root, 10), 4u);
}

TEST_F(PudSolverTest, OldestLeafIsTheOneConsidered) {
    give_leaf(4, {});
    give_leaf(9, body_goal_exprs_t{}.set(0, expr_at(0x10)));
    EXPECT_CALL(live_leaves, find_live_leaves(root)).WillOnce(Return(std::vector<pud_node_id>{9, 4}));
    EXPECT_EQ(solver.solve(root, 10), 4u);
}

TEST_F(PudSolverTest, OldestLeafIsExpandedAtItsFirstOpenGoal) {
    give_leaf(4, body_goal_exprs_t{}.set(7, expr_at(0x10)).set(3, expr_at(0x20)).set(5, expr_at(0x30)));
    give_leaf(5, {});
    EXPECT_CALL(live_leaves, find_live_leaves(root))
        .WillOnce(Return(std::vector<pud_node_id>{4}))
        .WillOnce(Return(std::vector<pud_node_id>{5}));
    EXPECT_CALL(expander, expand(4u, 3u));
    EXPECT_EQ(solver.solve(root, 10), 5u);
}

TEST_F(PudSolverTest, ZeroBudgetExpandsNothing) {
    give_leaf(4, body_goal_exprs_t{}.set(0, expr_at(0x10)));
    EXPECT_CALL(live_leaves, find_live_leaves(root)).WillOnce(Return(std::vector<pud_node_id>{4}));
    EXPECT_CALL(expander, expand(_, _)).Times(0);
    EXPECT_FALSE(solver.solve(root, 0).has_value());
}

TEST_F(PudSolverTest, ZeroBudgetStillFindsAnExistingSolution) {
    give_leaf(4, {});
    EXPECT_CALL(live_leaves, find_live_leaves(root)).WillOnce(Return(std::vector<pud_node_id>{4}));
    EXPECT_EQ(solver.solve(root, 0), 4u);
}

TEST_F(PudSolverTest, BudgetLimitsTheNumberOfExpansions) {
    give_leaf(4, body_goal_exprs_t{}.set(0, expr_at(0x10)));
    ON_CALL(live_leaves, find_live_leaves(root)).WillByDefault(Return(std::vector<pud_node_id>{4}));
    EXPECT_CALL(expander, expand(4u, 0u)).Times(3);
    EXPECT_FALSE(solver.solve(root, 3).has_value());
}

TEST_F(PudSolverTest, TreeRefutedMidwayEndsTheSearch) {
    give_leaf(4, body_goal_exprs_t{}.set(0, expr_at(0x10)));
    EXPECT_CALL(live_leaves, find_live_leaves(root))
        .WillOnce(Return(std::vector<pud_node_id>{4}))
        .WillOnce(Return(std::vector<pud_node_id>{}));
    EXPECT_CALL(expander, expand(4u, 0u)).Times(1);
    EXPECT_FALSE(solver.solve(root, 10).has_value());
}

}  // namespace
