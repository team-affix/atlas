#include <cstdint>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_eager_leaf_evaluator.hpp"
#include "pud_test_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_leaf_evaluation.hpp"
#include "value_objects/pud_ubs_state.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Key;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

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
struct MockQueryGoal {
    MOCK_METHOD(
        std::vector<pud_eager_candidate>, query, (pud_module_id, const pud_ubs_state&, const expr*, uint32_t));
};

using test_evaluator_t = pud_eager_leaf_evaluator<
    NiceMock<MockGetNodeGoals>, NiceMock<MockGetNodeBindings>, NiceMock<MockGetNodeLvc>, NiceMock<MockQueryGoal>>;

struct PudEagerLeafEvaluatorTest : public ::testing::Test {
    NiceMock<MockGetNodeGoals> goals;
    NiceMock<MockGetNodeBindings> bindings;
    NiceMock<MockGetNodeLvc> lvcs;
    NiceMock<MockQueryGoal> querier;
    test_evaluator_t evaluator{goals, bindings, lvcs, querier};
    expr goal_a{.content = expr::var{.index = 0}};
    expr goal_b{.content = expr::var{.index = 1}};
    expr goal_c{.content = expr::var{.index = 2}};
    pud_goal_list leaf_goals{
        .goals = immer::map<pud_body_goal_id, const expr*>{}.set(1, &goal_a).set(4, &goal_b).set(6, &goal_c),
        .next_id = 7};
    pud_ubs_state leaf_bindings{};
    pud_eager_candidate candidate{.path = {pud_test_node(9)}, .witness_leaves = {pud_test_node(9)}};

    void SetUp() override {
        ON_CALL(goals, get(pud_test_node(2))).WillByDefault(ReturnRef(leaf_goals));
        ON_CALL(bindings, get(pud_test_node(2))).WillByDefault(ReturnRef(leaf_bindings));
        ON_CALL(lvcs, get(pud_test_node(2))).WillByDefault(Return(5u));
        ON_CALL(querier, query(_, _, _, _)).WillByDefault(Return(std::vector<pud_eager_candidate>{candidate}));
    }
};

}

TEST_F(PudEagerLeafEvaluatorTest, EveryGoalIsQueriedInTheLeafsModuleUnderItsBindingsAndLvc) {
    EXPECT_CALL(querier, query(k_test_module, ::testing::Ref(leaf_bindings), &goal_a, 5u));
    EXPECT_CALL(querier, query(k_test_module, ::testing::Ref(leaf_bindings), &goal_b, 5u));
    EXPECT_CALL(querier, query(k_test_module, ::testing::Ref(leaf_bindings), &goal_c, 5u));
    evaluator.evaluate(pud_test_node(2));
}

TEST_F(PudEagerLeafEvaluatorTest, ALeafWhoseGoalsAllHaveCandidatesIsNotRefuted) {
    const pud_leaf_evaluation evaluation = evaluator.evaluate(pud_test_node(2));
    EXPECT_EQ(evaluation.leaf, pud_test_node(2));
    EXPECT_FALSE(evaluation.refuted);
    EXPECT_THAT(evaluation.goals, ElementsAre(Key(1u), Key(4u), Key(6u)));
    EXPECT_THAT(evaluation.goals.at(4), ElementsAre(candidate));
}

TEST_F(PudEagerLeafEvaluatorTest, AGoalWithoutCandidatesRefutesTheLeafAndLaterGoalsAreNotQueried) {
    EXPECT_CALL(querier, query(_, _, &goal_a, _)).WillOnce(Return(std::vector<pud_eager_candidate>{candidate}));
    EXPECT_CALL(querier, query(_, _, &goal_b, _)).WillOnce(Return(std::vector<pud_eager_candidate>{}));
    EXPECT_CALL(querier, query(_, _, &goal_c, _)).Times(0);
    const pud_leaf_evaluation evaluation = evaluator.evaluate(pud_test_node(2));
    EXPECT_TRUE(evaluation.refuted);
    EXPECT_THAT(evaluation.goals, ElementsAre(Key(1u), Key(4u)));
}
