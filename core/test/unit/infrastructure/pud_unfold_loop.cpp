#include <set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unfold_loop.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetOpenLeaves {
    MOCK_METHOD(const std::set<pud_node_id>&, get, ());
};
struct MockGenerateDecision {
    MOCK_METHOD(pud_decision, generate, (const std::set<pud_node_id>&));
};
struct MockUnfold {
    MOCK_METHOD(void, unfold, (pud_node_id, body_goal_id));
};

using test_unfold_loop_t = pud_unfold_loop<
    NiceMock<MockGetOpenLeaves>, NiceMock<MockGenerateDecision>, NiceMock<MockUnfold>>;

// The open set is a real std::set the tests edit; expanding a leaf closes it unless a test says otherwise.
struct PudUnfoldLoopTest : public ::testing::Test {
    NiceMock<MockGetOpenLeaves> open_leaves;
    NiceMock<MockGenerateDecision> decisions;
    NiceMock<MockUnfold> unfolder;
    test_unfold_loop_t unfold_loop{open_leaves, decisions, unfolder};
    std::set<pud_node_id> open;

    void SetUp() override {
        ON_CALL(open_leaves, get()).WillByDefault(ReturnRef(open));
        ON_CALL(decisions, generate(_)).WillByDefault([](const std::set<pud_node_id>& leaves) {
            return pud_decision{*leaves.begin(), 0};
        });
        ON_CALL(unfolder, unfold(_, _)).WillByDefault([this](pud_node_id leaf, body_goal_id) {
            open.erase(leaf);
        });
    }
};

}  // namespace

TEST_F(PudUnfoldLoopTest, NothingHappensBeforeTheFirstResume) {
    open = {4};
    EXPECT_CALL(decisions, generate(_)).Times(0);
    EXPECT_CALL(unfolder, unfold(_, _)).Times(0);
    coroutine<pud_decision, void> unfolding = unfold_loop.drive_unfold_loop();
}

TEST_F(PudUnfoldLoopTest, NoOpenLeavesEndsTheSearchWithoutAnyDecision) {
    EXPECT_CALL(decisions, generate(_)).Times(0);
    coroutine<pud_decision, void> unfolding = unfold_loop.drive_unfold_loop();
    unfolding.resume();
    EXPECT_FALSE(unfolding.has_yield());
    EXPECT_TRUE(unfolding.done());
}

TEST_F(PudUnfoldLoopTest, EveryOpenLeafIsOfferedToTheDecisionGenerator) {
    open = {4, 9, 12};
    EXPECT_CALL(decisions, generate(std::set<pud_node_id>{4, 9, 12})).WillOnce(Return(pud_decision{9, 1}));
    coroutine<pud_decision, void> unfolding = unfold_loop.drive_unfold_loop();
    unfolding.resume();
}

TEST_F(PudUnfoldLoopTest, DecisionIsExpandedAndThenYielded) {
    open = {4, 9};
    ON_CALL(decisions, generate(_)).WillByDefault(Return(pud_decision{9, 3}));
    EXPECT_CALL(unfolder, unfold(9, 3));
    coroutine<pud_decision, void> unfolding = unfold_loop.drive_unfold_loop();
    unfolding.resume();
    ASSERT_TRUE(unfolding.has_yield());
    EXPECT_EQ(unfolding.consume_yield(), (pud_decision{9, 3}));
}

TEST_F(PudUnfoldLoopTest, DecisionIsGeneratedBeforeItIsExpanded) {
    open = {4};
    InSequence in_order;
    EXPECT_CALL(decisions, generate(_)).WillOnce(Return(pud_decision{4, 0}));
    EXPECT_CALL(unfolder, unfold(4, 0));
    coroutine<pud_decision, void> unfolding = unfold_loop.drive_unfold_loop();
    unfolding.resume();
}

TEST_F(PudUnfoldLoopTest, EachResumeMakesExactlyOneDecision) {
    open = {4, 9, 12};
    coroutine<pud_decision, void> unfolding = unfold_loop.drive_unfold_loop();
    EXPECT_CALL(unfolder, unfold(4, 0));
    unfolding.resume();
    ::testing::Mock::VerifyAndClearExpectations(&unfolder);
    EXPECT_CALL(unfolder, unfold(9, 0));
    unfolding.resume();
}

TEST_F(PudUnfoldLoopTest, LeavesOpenedByAnExpansionAreOfferedNext) {
    open = {4};
    ON_CALL(unfolder, unfold(4, _)).WillByDefault([this](pud_node_id, body_goal_id) {
        open.erase(4);
        open.insert({20, 21});
    });
    coroutine<pud_decision, void> unfolding = unfold_loop.drive_unfold_loop();
    unfolding.resume();
    unfolding.consume_yield();
    EXPECT_CALL(decisions, generate(std::set<pud_node_id>{20, 21})).WillOnce(Return(pud_decision{21, 0}));
    unfolding.resume();
}

TEST_F(PudUnfoldLoopTest, SearchEndsOnceTheLastOpenLeafIsExpanded) {
    open = {4, 9};
    coroutine<pud_decision, void> unfolding = unfold_loop.drive_unfold_loop();
    unfolding.resume();
    unfolding.consume_yield();
    unfolding.resume();
    unfolding.consume_yield();
    unfolding.resume();
    EXPECT_FALSE(unfolding.has_yield());
    EXPECT_TRUE(unfolding.done());
}

TEST_F(PudUnfoldLoopTest, ExpansionThatKeepsTheLeafOpenOffersItAgain) {
    open = {4};
    ON_CALL(unfolder, unfold(_, _)).WillByDefault([](pud_node_id, body_goal_id) {});
    coroutine<pud_decision, void> unfolding = unfold_loop.drive_unfold_loop();
    for (int round = 0; round < 3; ++round) {
        unfolding.resume();
        ASSERT_TRUE(unfolding.has_yield());
        EXPECT_EQ(unfolding.consume_yield(), (pud_decision{4, 0}));
    }
}
