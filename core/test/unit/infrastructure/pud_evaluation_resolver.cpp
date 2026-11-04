#include <cstdint>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_evaluation_resolver.hpp"
#include "pud_test_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_leaf_evaluation.hpp"
#include "value_objects/pud_ubs_state.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetEvaluation {
    MOCK_METHOD(const pud_leaf_evaluation&, get, (), (const));
};
struct MockGetNodeBindings {
    MOCK_METHOD(const pud_ubs_state&, get, (pud_global_node_id));
};
struct MockGetNodeLvc {
    MOCK_METHOD(uint32_t, get, (pud_global_node_id));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id));
};
struct MockBuildInference {
    MOCK_METHOD(
        pud_inference,
        build,
        (pud_global_node_id, pud_body_goal_id, const pud_ubs_state&, uint32_t, const expr*,
         (const std::vector<pud_global_node_id>&)));
};

pud_inference inference_with_var_count(uint32_t var_count) {
    return pud_inference{.parent = 2, .call_site = 3, .substitutions = {}, .body_goals = {}, .var_count = var_count};
}

using test_resolver_t = pud_evaluation_resolver<
    NiceMock<MockGetEvaluation>, NiceMock<MockGetNodeBindings>, NiceMock<MockGetNodeLvc>, NiceMock<MockGetNodeGoals>,
    NiceMock<MockBuildInference>>;

struct PudEvaluationResolverTest : public ::testing::Test {
    NiceMock<MockGetEvaluation> evaluations;
    NiceMock<MockGetNodeBindings> bindings;
    NiceMock<MockGetNodeLvc> lvcs;
    NiceMock<MockGetNodeGoals> goals;
    NiceMock<MockBuildInference> builder;
    test_resolver_t resolver{evaluations, bindings, lvcs, goals, builder};
    expr goal{.content = expr::var{.index = 0}};
    pud_goal_list leaf_goals{.goals = immer::map<pud_body_goal_id, const expr*>{}.set(3, &goal), .next_id = 4};
    pud_ubs_state leaf_bindings{};
    std::vector<pud_global_node_id> first_path{pud_test_node(10), pud_test_node(11)};
    std::vector<pud_global_node_id> second_path{pud_test_node(20)};
    pud_leaf_evaluation evaluation{
        .leaf = pud_test_node(2),
        .refuted = false,
        .goals = {
            {1, {pud_eager_candidate{.path = {pud_test_node(30)}, .witness_leaves = {pud_test_node(30)}}}},
            {3,
             {pud_eager_candidate{.path = first_path, .witness_leaves = {pud_test_node(12), pud_test_node(13)}},
              pud_eager_candidate{.path = second_path, .witness_leaves = {pud_test_node(20)}}}},
        }};

    void SetUp() override {
        ON_CALL(evaluations, get()).WillByDefault(ReturnRef(evaluation));
        ON_CALL(bindings, get(pud_test_node(2))).WillByDefault(ReturnRef(leaf_bindings));
        ON_CALL(lvcs, get(pud_test_node(2))).WillByDefault(Return(7u));
        ON_CALL(goals, get(pud_test_node(2))).WillByDefault(ReturnRef(leaf_goals));
    }
};

}

TEST_F(PudEvaluationResolverTest, EachCandidateOfTheGoalIsBuiltAlongItsPathInOrder) {
    InSequence in_order;
    EXPECT_CALL(builder, build(pud_test_node(2), 3u, ::testing::Ref(leaf_bindings), 7u, &goal, first_path))
        .WillOnce(Return(inference_with_var_count(1)));
    EXPECT_CALL(builder, build(pud_test_node(2), 3u, ::testing::Ref(leaf_bindings), 7u, &goal, second_path))
        .WillOnce(Return(inference_with_var_count(2)));
    EXPECT_THAT(
        resolver.resolve(pud_test_node(2), 3),
        ElementsAre(inference_with_var_count(1), inference_with_var_count(2)));
}

TEST_F(PudEvaluationResolverTest, OtherGoalsCandidatesAreNotBuilt) {
    EXPECT_CALL(builder, build(_, _, _, _, _, ElementsAre(pud_test_node(30)))).Times(0);
    EXPECT_CALL(builder, build(_, _, _, _, _, first_path));
    EXPECT_CALL(builder, build(_, _, _, _, _, second_path));
    resolver.resolve(pud_test_node(2), 3);
}
