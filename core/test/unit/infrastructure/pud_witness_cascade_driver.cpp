#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_witness_cascade_driver.hpp"
#include "pud_test_node.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_leaf_evaluation.hpp"
#include "value_objects/pud_unfold_outcome.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockNextFocusLeaf {
    MOCK_METHOD(std::optional<pud_global_node_id>, next, ());
};
struct MockIsUnfoldable {
    MOCK_METHOD(bool, is_unfoldable, (pud_global_node_id), (const));
};
struct MockEvaluateLeaf {
    MOCK_METHOD(pud_leaf_evaluation, evaluate, (pud_global_node_id), (const));
};
struct MockStoreEvaluation {
    MOCK_METHOD(void, store, (pud_leaf_evaluation));
};
struct MockRefuteLeaf {
    MOCK_METHOD(std::vector<pud_global_node_id>, refute, (pud_global_node_id));
};
struct MockEraseRefuted {
    MOCK_METHOD(void, erase_refuted, (const std::vector<pud_global_node_id>&));
};
struct MockChooseGoal {
    MOCK_METHOD(pud_body_goal_id, choose, (pud_global_node_id), (const));
};
struct MockUnfoldLeaf {
    MOCK_METHOD(pud_unfold_outcome, unfold, (pud_global_node_id, pud_body_goal_id));
};

// Every visited leaf has goal 0 with one candidate, witnessed by the leaves ten and eleven past it.
pud_leaf_evaluation witnessed_by_next_leaves(pud_global_node_id leaf) {
    const pud_global_node_id first{.module = leaf.module, .node = leaf.node + 10};
    const pud_global_node_id second{.module = leaf.module, .node = leaf.node + 11};
    return pud_leaf_evaluation{
        .leaf = leaf,
        .refuted = false,
        .goals = {{0, {pud_eager_candidate{.path = {first}, .witness_leaves = {first, second}}}}}};
}

using test_driver_t = pud_witness_cascade_driver<
    NiceMock<MockNextFocusLeaf>, NiceMock<MockIsUnfoldable>, NiceMock<MockEvaluateLeaf>,
    NiceMock<MockStoreEvaluation>, NiceMock<MockRefuteLeaf>, NiceMock<MockEraseRefuted>, NiceMock<MockChooseGoal>,
    NiceMock<MockUnfoldLeaf>>;

struct PudWitnessCascadeDriverTest : public ::testing::Test {
    NiceMock<MockNextFocusLeaf> focus;
    NiceMock<MockIsUnfoldable> unfoldable;
    NiceMock<MockEvaluateLeaf> evaluator;
    NiceMock<MockStoreEvaluation> store;
    NiceMock<MockRefuteLeaf> refuter;
    NiceMock<MockEraseRefuted> frontier;
    NiceMock<MockChooseGoal> chooser;
    NiceMock<MockUnfoldLeaf> unfolder;

    test_driver_t make_driver(size_t cascade_depth) {
        return test_driver_t{
            focus, unfoldable, evaluator, store, refuter, frontier, chooser, unfolder, cascade_depth};
    }

    void SetUp() override {
        ON_CALL(focus, next()).WillByDefault(Return(pud_test_node(1)));
        ON_CALL(unfoldable, is_unfoldable(_)).WillByDefault(Return(true));
        ON_CALL(evaluator, evaluate(_)).WillByDefault(Invoke(&witnessed_by_next_leaves));
        ON_CALL(chooser, choose(_)).WillByDefault(Return(0u));
        ON_CALL(unfolder, unfold(_, _)).WillByDefault(Return(pud_unfold_outcome::on_focus));
    }
};

}

TEST_F(PudWitnessCascadeDriverTest, AFocusLeafIsEvaluatedThenUnfoldedOnTheGoalChosenFromItsEvaluation) {
    test_driver_t driver = make_driver(0);
    InSequence in_order;
    EXPECT_CALL(evaluator, evaluate(pud_test_node(1)));
    EXPECT_CALL(store, store(witnessed_by_next_leaves(pud_test_node(1))));
    EXPECT_CALL(chooser, choose(pud_test_node(1))).WillOnce(Return(0u));
    EXPECT_CALL(unfolder, unfold(pud_test_node(1), 0u)).WillOnce(Return(pud_unfold_outcome::off_focus));
    EXPECT_EQ(driver.unfold_one(), pud_unfold_outcome::off_focus);
}

TEST_F(PudWitnessCascadeDriverTest, AtCascadeDepthZeroOnlyFocusLeavesAreUnfolded) {
    test_driver_t driver = make_driver(0);
    EXPECT_CALL(focus, next()).WillOnce(Return(pud_test_node(1))).WillOnce(Return(pud_test_node(2)));
    EXPECT_CALL(unfolder, unfold(pud_test_node(1), _));
    EXPECT_CALL(unfolder, unfold(pud_test_node(2), _));
    driver.unfold_one();
    driver.unfold_one();
}

TEST_F(PudWitnessCascadeDriverTest, TheChosenGoalsWitnessesAreUnfoldedBeforeTheNextFocusLeaf) {
    test_driver_t driver = make_driver(1);
    EXPECT_CALL(focus, next()).WillOnce(Return(pud_test_node(1))).WillOnce(Return(pud_test_node(2)));
    InSequence in_order;
    EXPECT_CALL(unfolder, unfold(pud_test_node(1), _));
    EXPECT_CALL(unfolder, unfold(pud_test_node(11), _));
    EXPECT_CALL(unfolder, unfold(pud_test_node(12), _));
    EXPECT_CALL(unfolder, unfold(pud_test_node(2), _));
    for (int unfold = 0; unfold < 4; ++unfold)
        driver.unfold_one();
}

TEST_F(PudWitnessCascadeDriverTest, WitnessesOfWitnessesAreUnfoldedDownToTheCascadeDepth) {
    test_driver_t driver = make_driver(2);
    EXPECT_CALL(focus, next()).WillOnce(Return(pud_test_node(1))).WillOnce(Return(pud_test_node(2)));
    InSequence in_order;
    EXPECT_CALL(unfolder, unfold(pud_test_node(1), _));
    EXPECT_CALL(unfolder, unfold(pud_test_node(11), _));
    EXPECT_CALL(unfolder, unfold(pud_test_node(12), _));
    EXPECT_CALL(unfolder, unfold(pud_test_node(21), _));
    EXPECT_CALL(unfolder, unfold(pud_test_node(22), _));
    EXPECT_CALL(unfolder, unfold(pud_test_node(22), _));
    EXPECT_CALL(unfolder, unfold(pud_test_node(23), _));
    EXPECT_CALL(unfolder, unfold(pud_test_node(2), _));
    for (int unfold = 0; unfold < 8; ++unfold)
        driver.unfold_one();
}

TEST_F(PudWitnessCascadeDriverTest, QueuedWitnessesThatCanNoLongerBeUnfoldedArePassedOver) {
    test_driver_t driver = make_driver(1);
    ON_CALL(unfoldable, is_unfoldable(pud_test_node(11))).WillByDefault(Return(false));
    EXPECT_CALL(evaluator, evaluate(_)).Times(AnyNumber());
    EXPECT_CALL(evaluator, evaluate(pud_test_node(11))).Times(0);
    InSequence in_order;
    EXPECT_CALL(unfolder, unfold(pud_test_node(1), _));
    EXPECT_CALL(unfolder, unfold(pud_test_node(12), _));
    driver.unfold_one();
    driver.unfold_one();
}

TEST_F(PudWitnessCascadeDriverTest, ALeafWithAGoalWithoutCandidatesIsRefutedAndTheNextLeafIsUnfolded) {
    test_driver_t driver = make_driver(0);
    EXPECT_CALL(focus, next()).WillOnce(Return(pud_test_node(1))).WillOnce(Return(pud_test_node(2)));
    ON_CALL(evaluator, evaluate(pud_test_node(1)))
        .WillByDefault(Return(pud_leaf_evaluation{.leaf = pud_test_node(1), .refuted = true, .goals = {{0, {}}}}));
    EXPECT_CALL(refuter, refute(pud_test_node(1)))
        .WillOnce(Return(std::vector<pud_global_node_id>{pud_test_node(1), pud_test_node(0)}));
    EXPECT_CALL(frontier, erase_refuted(ElementsAre(pud_test_node(1), pud_test_node(0))));
    EXPECT_CALL(unfolder, unfold(pud_test_node(1), _)).Times(0);
    EXPECT_CALL(unfolder, unfold(pud_test_node(2), _)).WillOnce(Return(pud_unfold_outcome::on_focus));
    EXPECT_EQ(driver.unfold_one(), pud_unfold_outcome::on_focus);
}

TEST_F(PudWitnessCascadeDriverTest, NothingIsUnfoldedWhenNoFocusLeafIsLeft) {
    test_driver_t driver = make_driver(1);
    EXPECT_CALL(focus, next()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(unfolder, unfold(_, _)).Times(0);
    EXPECT_EQ(driver.unfold_one(), pud_unfold_outcome::none);
}
