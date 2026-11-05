#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unit_goal_first_chooser.hpp"
#include "pud_test_node.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_leaf_evaluation.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetEvaluation {
    MOCK_METHOD(const pud_leaf_evaluation&, get, (), (const));
};
struct MockChooseGoal {
    MOCK_METHOD(pud_body_goal_id, choose, (pud_global_node_id), (const));
};

pud_eager_candidate candidate_at(pud_node_id node) {
    return pud_eager_candidate{.path = {pud_test_node(node)}, .witness_leaves = {pud_test_node(node)}};
}

struct PudUnitGoalFirstChooserTest : public ::testing::Test {
    NiceMock<MockGetEvaluation> evaluations;
    NiceMock<MockChooseGoal> fallback;
    pud_unit_goal_first_chooser<NiceMock<MockGetEvaluation>, NiceMock<MockChooseGoal>> chooser{evaluations, fallback};
    pud_leaf_evaluation evaluation{.leaf = pud_test_node(1), .refuted = false, .goals = {}};

    void SetUp() override {
        ON_CALL(evaluations, get()).WillByDefault(ReturnRef(evaluation));
        ON_CALL(fallback, choose(_)).WillByDefault(Return(9u));
    }
};

}

TEST_F(PudUnitGoalFirstChooserTest, TheSmallestUnitGoalIsChosen) {
    evaluation.goals = {
        {2, {candidate_at(10), candidate_at(11)}},
        {4, {candidate_at(12)}},
        {6, {candidate_at(13)}},
    };
    EXPECT_CALL(fallback, choose(_)).Times(0);
    EXPECT_EQ(chooser.choose(pud_test_node(1)), 4u);
}

TEST_F(PudUnitGoalFirstChooserTest, WithoutAUnitGoalTheFallbackChooses) {
    evaluation.goals = {
        {2, {candidate_at(10), candidate_at(11)}},
        {4, {candidate_at(12), candidate_at(13)}},
    };
    EXPECT_CALL(fallback, choose(pud_test_node(1))).WillOnce(Return(9u));
    EXPECT_EQ(chooser.choose(pud_test_node(1)), 9u);
}
