#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_leaf_query_maker.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockOpenQuery {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, try_open, (pud_node_id, const pud_ubs_state&, const expr*, uint32_t));
};
struct MockForkQuery {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, try_fork, (pud_mhq_head_id, const pud_ubs_state&, const expr*, uint32_t));
};
struct MockHoldQuery {
    MOCK_METHOD(void, hold, (pud_mhq_head_id, pud_node_id, pud_body_goal_id));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_node_id));
    MOCK_METHOD(const expr*, find, (pud_node_id, pud_body_goal_id));
};
struct MockGetNodeLvc {
    MOCK_METHOD(uint32_t, get, (pud_node_id));
};

using queries_t = immer::map<pud_body_goal_id, pud_mhq_head_id>;
using test_query_maker_t = pud_leaf_query_maker<
    NiceMock<MockOpenQuery>, NiceMock<MockForkQuery>, NiceMock<MockHoldQuery>, NiceMock<MockGetNodeGoals>, NiceMock<MockGetNodeLvc>>;

struct PudLeafQueryMakerTest : public ::testing::Test {
    NiceMock<MockOpenQuery> open;
    NiceMock<MockForkQuery> fork;
    NiceMock<MockHoldQuery> hold;
    NiceMock<MockGetNodeGoals> goals;
    NiceMock<MockGetNodeLvc> lvcs;
    test_query_maker_t maker{open, fork, hold, goals, lvcs};
    expr goal_a{expr::functor{1, {}}};
    expr goal_b{expr::functor{2, {}}};
    expr goal_c{expr::functor{3, {}}};
    pud_goal_list root_goals = pud_goal_list{.goals = {}, .next_id = 0}.adding(&goal_a).adding(&goal_b);
    pud_goal_list child_goals = pud_goal_list{.goals = {}, .next_id = 0}.adding(&goal_a).adding(&goal_b).adding(&goal_c).closing(0);
    queries_t parent_queries = queries_t{}.set(0, 40).set(1, 41).set(2, 42);
    pud_ubs_state bindings;

    void SetUp() override {
        ON_CALL(goals, get(1)).WillByDefault(ReturnRef(root_goals));
        ON_CALL(goals, get(2)).WillByDefault(ReturnRef(child_goals));
        ON_CALL(goals, find(2, 2)).WillByDefault(Return(&goal_c));
        ON_CALL(lvcs, get(_)).WillByDefault(Return(6));
        ON_CALL(open, try_open(_, _, &goal_a, _)).WillByDefault(Return(50));
        ON_CALL(open, try_open(_, _, &goal_b, _)).WillByDefault(Return(51));
    }
};

}

TEST_F(PudLeafQueryMakerTest, ARootOpensAQueryForEveryGoalAtItsLvcAndHoldsThem) {
    EXPECT_CALL(open, try_open(1u, _, &goal_a, 6u)).WillOnce(Return(50));
    EXPECT_CALL(open, try_open(1u, _, &goal_b, 6u)).WillOnce(Return(51));
    EXPECT_CALL(hold, hold(50u, 1u, 0u));
    EXPECT_CALL(hold, hold(51u, 1u, 1u));
    EXPECT_EQ(maker.make_root_queries(1, bindings), (pud_goal_queries{.heads = queries_t{}.set(0, 50).set(1, 51), .refuted = false}));
}

TEST_F(PudLeafQueryMakerTest, AGoalWithoutCandidatesRefutesTheLeafButTheOthersAreStillHeld) {
    ON_CALL(open, try_open(_, _, &goal_a, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(hold, hold(51u, 1u, 1u));
    EXPECT_EQ(maker.make_root_queries(1, bindings), (pud_goal_queries{.heads = queries_t{}.set(1, 51), .refuted = true}));
}

TEST_F(PudLeafQueryMakerTest, AChildSharesTheParentsQueryForAnUnchangedGoalAndDropsTheCallSite) {
    EXPECT_CALL(open, try_open(_, _, _, _)).Times(0);
    EXPECT_CALL(fork, try_fork(_, _, _, _)).Times(0);
    EXPECT_CALL(hold, hold(41u, 2u, 1u));
    EXPECT_CALL(hold, hold(42u, 2u, 2u));
    pud_goal_queries made = maker.make_child_queries(parent_queries, 0);
    maker.admit_child_queries(made, 2, bindings, parent_queries);
    EXPECT_EQ(made.heads, (queries_t{}.set(1, 41).set(2, 42)));
}

TEST_F(PudLeafQueryMakerTest, AChangedGoalForksTheParentsQueryInTheChildsEnvironment) {
    EXPECT_CALL(fork, try_fork(42u, _, &goal_c, 6u)).WillOnce(Return(60));
    EXPECT_CALL(hold, hold(41u, 2u, 1u));
    EXPECT_CALL(hold, hold(60u, 2u, 2u));
    pud_goal_queries made = maker.make_child_queries(parent_queries, 0);
    maker.fork_changed_goal(made, 2, bindings, 2);
    maker.admit_child_queries(made, 2, bindings, parent_queries);
    EXPECT_EQ(made.heads, (queries_t{}.set(1, 41).set(2, 60)));
    EXPECT_FALSE(made.refuted);
}

TEST_F(PudLeafQueryMakerTest, AForkWithoutCandidatesRefutesTheChild) {
    EXPECT_CALL(fork, try_fork(42u, _, _, _)).WillOnce(Return(std::nullopt));
    pud_goal_queries made = maker.make_child_queries(parent_queries, 0);
    maker.fork_changed_goal(made, 2, bindings, 2);
    maker.admit_child_queries(made, 2, bindings, parent_queries);
    EXPECT_TRUE(made.refuted);
    EXPECT_EQ(made.heads, (queries_t{}.set(1, 41)));
}

TEST_F(PudLeafQueryMakerTest, AGoalTheChildAddsGetsAQueryOfItsOwn) {
    const queries_t inherited = queries_t{}.set(0, 40).set(1, 41);
    EXPECT_CALL(open, try_open(2u, _, &goal_c, 6u)).WillOnce(Return(70));
    EXPECT_CALL(hold, hold(41u, 2u, 1u));
    EXPECT_CALL(hold, hold(70u, 2u, 2u));
    pud_goal_queries made = maker.make_child_queries(inherited, 0);
    maker.admit_child_queries(made, 2, bindings, inherited);
    EXPECT_EQ(made.heads, (queries_t{}.set(1, 41).set(2, 70)));
}
