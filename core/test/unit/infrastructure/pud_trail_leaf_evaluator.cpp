#include <cstdint>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_trail_leaf_evaluator.hpp"
#include "pud_test_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_goal_trails.hpp"
#include "value_objects/pud_leaf_evaluation.hpp"
#include "value_objects/pud_node_trails.hpp"
#include "value_objects/pud_ubs_state.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Key;
using ::testing::NiceMock;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;

namespace {

struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id));
};
struct MockGetNodeBindings {
    MOCK_METHOD(const pud_ubs_state&, get, (pud_global_node_id));
};
struct MockGetNodeLvc {
    MOCK_METHOD(uint32_t, get, (pud_global_node_id));
};
struct MockGetNodeTrails {
    MOCK_METHOD(const pud_node_trails&, get, (pud_global_node_id));
};
struct MockEraseNodeTrails {
    MOCK_METHOD(void, erase, (pud_global_node_id));
};
struct MockStoreNodeTrails {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_node_trails));
};
struct MockQueryGoal {
    MOCK_METHOD(
        std::vector<pud_eager_candidate>,
        query,
        (pud_module_id, const pud_ubs_state&, const expr*, uint32_t, const pud_goal_trails*));
};

using test_evaluator_t = pud_trail_leaf_evaluator<
    NiceMock<MockGetNodeGoals>, NiceMock<MockGetNodeBindings>, NiceMock<MockGetNodeLvc>, NiceMock<MockGetNodeTrails>,
    NiceMock<MockEraseNodeTrails>, NiceMock<MockStoreNodeTrails>, NiceMock<MockQueryGoal>>;

// The leaf 2 has goals 1 and 4; it holds trails for goal 1 only, and every query finds candidate 9.
struct PudTrailLeafEvaluatorTest : public ::testing::Test {
    NiceMock<MockGetNodeGoals> goals;
    NiceMock<MockGetNodeBindings> bindings;
    NiceMock<MockGetNodeLvc> lvcs;
    NiceMock<MockGetNodeTrails> get_trails;
    NiceMock<MockEraseNodeTrails> erase_trails;
    NiceMock<MockStoreNodeTrails> store_trails;
    NiceMock<MockQueryGoal> querier;
    test_evaluator_t evaluator{goals, bindings, lvcs, get_trails, erase_trails, store_trails, querier};
    expr goal_a{.content = expr::var{.index = 0}};
    expr goal_b{.content = expr::var{.index = 1}};
    pud_goal_list leaf_goals{
        .goals = immer::map<pud_body_goal_id, const expr*>{}.set(1, &goal_a).set(4, &goal_b), .next_id = 5};
    pud_ubs_state leaf_bindings{};
    pud_eager_candidate candidate{.path = {pud_test_node(0), pud_test_node(9)}, .witness_leaves = {pud_test_node(9)}};
    pud_goal_trails left_by_candidate{
        .candidates = {pud_candidate_trail{.candidate = pud_test_node(9), .witness_leaves = {pud_test_node(9)}}},
        .forked = false};
    pud_goal_trails held_for_a{.candidates = {}, .forked = true};
    pud_node_trails held = pud_node_trails{}.set(1, held_for_a);
    pud_node_trails stored{};

    void SetUp() override {
        ON_CALL(goals, get(pud_test_node(2))).WillByDefault(ReturnRef(leaf_goals));
        ON_CALL(bindings, get(pud_test_node(2))).WillByDefault(ReturnRef(leaf_bindings));
        ON_CALL(lvcs, get(pud_test_node(2))).WillByDefault(Return(5u));
        ON_CALL(get_trails, get(pud_test_node(2))).WillByDefault(ReturnRef(held));
        ON_CALL(querier, query(_, _, _, _, _)).WillByDefault(Return(std::vector<pud_eager_candidate>{candidate}));
        ON_CALL(store_trails, store(pud_test_node(2), _)).WillByDefault(SaveArg<1>(&stored));
    }
};

}

TEST_F(PudTrailLeafEvaluatorTest, EachGoalIsQueriedFromTheTrailsTheLeafHoldsForIt) {
    EXPECT_CALL(querier, query(k_test_module, ::testing::Ref(leaf_bindings), &goal_a, 5u, Pointee(held_for_a)));
    EXPECT_CALL(querier, query(k_test_module, ::testing::Ref(leaf_bindings), &goal_b, 5u, nullptr));
    evaluator.evaluate(pud_test_node(2));
}

TEST_F(PudTrailLeafEvaluatorTest, TheEvaluationListsEachGoalsCandidates) {
    const pud_leaf_evaluation evaluation = evaluator.evaluate(pud_test_node(2));
    EXPECT_FALSE(evaluation.refuted);
    EXPECT_THAT(evaluation.goals, ElementsAre(Key(1u), Key(4u)));
    EXPECT_THAT(evaluation.goals.at(4), ElementsAre(candidate));
}

TEST_F(PudTrailLeafEvaluatorTest, TheLeafThenHoldsTheUnforkedTrailsTheEvaluationLeft) {
    evaluator.evaluate(pud_test_node(2));
    ASSERT_NE(stored.find(1), nullptr);
    ASSERT_NE(stored.find(4), nullptr);
    EXPECT_EQ(stored.find(1)->get(), left_by_candidate);
    EXPECT_EQ(stored.find(4)->get(), left_by_candidate);
}

TEST_F(PudTrailLeafEvaluatorTest, TheNewTrailsReplaceTheHeldOnes) {
    ::testing::InSequence in_order;
    EXPECT_CALL(erase_trails, erase(pud_test_node(2)));
    EXPECT_CALL(store_trails, store(pud_test_node(2), _));
    evaluator.evaluate(pud_test_node(2));
}

TEST_F(PudTrailLeafEvaluatorTest, TrailsThatComeOutUnchangedKeepTheirBox) {
    held = pud_node_trails{}.set(1, left_by_candidate);
    evaluator.evaluate(pud_test_node(2));
    EXPECT_EQ(&stored.find(1)->get(), &held.find(1)->get());
}

TEST_F(PudTrailLeafEvaluatorTest, AGoalWithoutCandidatesRefutesTheLeafAndLaterGoalsAreNotQueried) {
    EXPECT_CALL(querier, query(_, _, &goal_a, _, _)).WillOnce(Return(std::vector<pud_eager_candidate>{}));
    EXPECT_CALL(querier, query(_, _, &goal_b, _, _)).Times(0);
    const pud_leaf_evaluation evaluation = evaluator.evaluate(pud_test_node(2));
    EXPECT_TRUE(evaluation.refuted);
    EXPECT_THAT(evaluation.goals, ElementsAre(Key(1u)));
}
