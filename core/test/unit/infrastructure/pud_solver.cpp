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

TEST_F(PudSolverTest, NothingHappensBeforeTheFirstResume) {
    EXPECT_CALL(live_leaves, find_live_leaves(_)).Times(0);
    auto search = solver.solve(root);
}

TEST_F(PudSolverTest, RefutedTreeEndsTheSearchWithoutYielding) {
    EXPECT_CALL(live_leaves, find_live_leaves(root)).WillOnce(Return(std::vector<pud_node_id>{}));
    auto search = solver.solve(root);
    search.resume();
    EXPECT_FALSE(search.has_yield());
    EXPECT_TRUE(search.done());
}

TEST_F(PudSolverTest, LeafWithNoOpenGoalsIsYieldedAsASolution) {
    give_leaf(4, {});
    ON_CALL(live_leaves, find_live_leaves(root)).WillByDefault(Return(std::vector<pud_node_id>{4}));
    EXPECT_CALL(expander, expand(_, _)).Times(0);
    auto search = solver.solve(root);
    search.resume();
    ASSERT_TRUE(search.has_yield());
    EXPECT_EQ(search.consume_yield(), pud_solver_step{pud_solution{4}});
}

TEST_F(PudSolverTest, SolutionIsYieldedOnlyOnce) {
    give_leaf(4, {});
    ON_CALL(live_leaves, find_live_leaves(root)).WillByDefault(Return(std::vector<pud_node_id>{4}));
    auto search = solver.solve(root);
    search.resume();
    ASSERT_TRUE(search.has_yield());
    search.consume_yield();
    search.resume();
    EXPECT_FALSE(search.has_yield());
    EXPECT_TRUE(search.done());
}

TEST_F(PudSolverTest, YieldedSolutionsAreSkippedForTheNextOldestLeaf) {
    give_leaf(4, {});
    give_leaf(5, body_goal_exprs_t{}.set(1, expr_at(0x10)));
    ON_CALL(live_leaves, find_live_leaves(root)).WillByDefault(Return(std::vector<pud_node_id>{5, 4}));
    auto search = solver.solve(root);
    search.resume();
    EXPECT_EQ(search.consume_yield(), pud_solver_step{pud_solution{4}});
    EXPECT_CALL(expander, expand(5u, 1u));
    search.resume();
    EXPECT_EQ(search.consume_yield(), (pud_solver_step{pud_unfolded{5, 1}}));
}

TEST_F(PudSolverTest, YieldedSolutionsAreNeverRefuted) {
    give_leaf(4, {});
    ON_CALL(live_leaves, find_live_leaves(root)).WillByDefault(Return(std::vector<pud_node_id>{4}));
    EXPECT_CALL(refuter, refute(_)).Times(0);
    auto search = solver.solve(root);
    search.resume();
    search.consume_yield();
    search.resume();
}

TEST_F(PudSolverTest, OldestLeafIsTheOneConsidered) {
    give_leaf(4, {});
    give_leaf(9, body_goal_exprs_t{}.set(0, expr_at(0x10)));
    ON_CALL(live_leaves, find_live_leaves(root)).WillByDefault(Return(std::vector<pud_node_id>{9, 4}));
    auto search = solver.solve(root);
    search.resume();
    EXPECT_EQ(search.consume_yield(), pud_solver_step{pud_solution{4}});
}

TEST_F(PudSolverTest, OldestLeafIsUnfoldedAtItsFirstOpenGoal) {
    give_leaf(4, body_goal_exprs_t{}.set(7, expr_at(0x10)).set(3, expr_at(0x20)).set(5, expr_at(0x30)));
    ON_CALL(live_leaves, find_live_leaves(root)).WillByDefault(Return(std::vector<pud_node_id>{4}));
    EXPECT_CALL(expander, expand(4u, 3u));
    auto search = solver.solve(root);
    search.resume();
    EXPECT_EQ(search.consume_yield(), (pud_solver_step{pud_unfolded{4, 3}}));
}

TEST_F(PudSolverTest, EachResumeUnfoldsExactlyOnce) {
    give_leaf(4, body_goal_exprs_t{}.set(0, expr_at(0x10)));
    ON_CALL(live_leaves, find_live_leaves(root)).WillByDefault(Return(std::vector<pud_node_id>{4}));
    auto search = solver.solve(root);
    for (int resume = 1; resume <= 3; ++resume) {
        EXPECT_CALL(expander, expand(4u, 0u)).Times(1);
        search.resume();
        ASSERT_TRUE(search.has_yield());
        search.consume_yield();
        ::testing::Mock::VerifyAndClearExpectations(&expander);
    }
}

TEST_F(PudSolverTest, TreeRefutedMidwayEndsTheSearch) {
    give_leaf(4, body_goal_exprs_t{}.set(0, expr_at(0x10)));
    EXPECT_CALL(live_leaves, find_live_leaves(root))
        .WillOnce(Return(std::vector<pud_node_id>{4}))
        .WillOnce(Return(std::vector<pud_node_id>{}));
    EXPECT_CALL(expander, expand(4u, 0u)).Times(1);
    auto search = solver.solve(root);
    search.resume();
    ASSERT_TRUE(search.has_yield());
    search.consume_yield();
    search.resume();
    EXPECT_TRUE(search.done());
}

TEST_F(PudSolverTest, IndependentSearchesKeepTheirOwnYieldedSolutions) {
    give_leaf(4, {});
    ON_CALL(live_leaves, find_live_leaves(root)).WillByDefault(Return(std::vector<pud_node_id>{4}));
    auto first = solver.solve(root);
    first.resume();
    first.consume_yield();
    auto second = solver.solve(root);
    second.resume();
    ASSERT_TRUE(second.has_yield());
    EXPECT_EQ(second.consume_yield(), pud_solver_step{pud_solution{4}});
}

}  // namespace
