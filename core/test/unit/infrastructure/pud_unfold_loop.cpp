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
struct MockGetNextUnfold {
    MOCK_METHOD(pud_unfold, get_next_unfold, (const std::set<pud_node_id>&));
};
struct MockExpand {
    MOCK_METHOD(std::vector<pud_node_id>, expand, (pud_node_id, pud_body_goal_id));
};

using test_unfold_loop_t = pud_unfold_loop<
    NiceMock<MockGetOpenLeaves>, NiceMock<MockGetNextUnfold>, NiceMock<MockExpand>>;

struct PudUnfoldLoopTest : public ::testing::Test {
    NiceMock<MockGetOpenLeaves> open_leaves;
    NiceMock<MockGetNextUnfold> next_unfolds;
    NiceMock<MockExpand> expander;
    test_unfold_loop_t unfold_loop{open_leaves, next_unfolds, expander};
    std::set<pud_node_id> open;

    void SetUp() override {
        ON_CALL(open_leaves, get()).WillByDefault(ReturnRef(open));
        ON_CALL(next_unfolds, get_next_unfold(_)).WillByDefault([](const std::set<pud_node_id>& leaves) {
            return pud_unfold{*leaves.begin(), 0};
        });
        ON_CALL(expander, expand(_, _)).WillByDefault([this](pud_node_id leaf, pud_body_goal_id) {
            open.erase(leaf);
            return std::vector<pud_node_id>{};
        });
    }
};

}

TEST_F(PudUnfoldLoopTest, NothingHappensBeforeTheFirstResume) {
    open = {4};
    EXPECT_CALL(next_unfolds, get_next_unfold(_)).Times(0);
    EXPECT_CALL(expander, expand(_, _)).Times(0);
    coroutine<pud_unfold, void> unfolding = unfold_loop.drive_unfold_loop();
}

TEST_F(PudUnfoldLoopTest, NoOpenLeavesEndsTheLoopWithoutAnyUnfold) {
    EXPECT_CALL(next_unfolds, get_next_unfold(_)).Times(0);
    coroutine<pud_unfold, void> unfolding = unfold_loop.drive_unfold_loop();
    unfolding.resume();
    EXPECT_FALSE(unfolding.has_yield());
    EXPECT_TRUE(unfolding.done());
}

TEST_F(PudUnfoldLoopTest, EveryOpenLeafIsOfferedToTheNextUnfold) {
    open = {4, 9, 12};
    EXPECT_CALL(next_unfolds, get_next_unfold(std::set<pud_node_id>{4, 9, 12})).WillOnce(Return(pud_unfold{9, 1}));
    coroutine<pud_unfold, void> unfolding = unfold_loop.drive_unfold_loop();
    unfolding.resume();
}

TEST_F(PudUnfoldLoopTest, ChosenUnfoldIsUnfoldedAndThenYielded) {
    open = {4, 9};
    ON_CALL(next_unfolds, get_next_unfold(_)).WillByDefault(Return(pud_unfold{9, 3}));
    EXPECT_CALL(expander, expand(9, 3));
    coroutine<pud_unfold, void> unfolding = unfold_loop.drive_unfold_loop();
    unfolding.resume();
    ASSERT_TRUE(unfolding.has_yield());
    EXPECT_EQ(unfolding.consume_yield(), (pud_unfold{9, 3}));
}

TEST_F(PudUnfoldLoopTest, UnfoldIsChosenBeforeItIsUnfolded) {
    open = {4};
    InSequence in_order;
    EXPECT_CALL(next_unfolds, get_next_unfold(_)).WillOnce(Return(pud_unfold{4, 0}));
    EXPECT_CALL(expander, expand(4, 0));
    coroutine<pud_unfold, void> unfolding = unfold_loop.drive_unfold_loop();
    unfolding.resume();
}

TEST_F(PudUnfoldLoopTest, EachResumeMakesExactlyOneUnfold) {
    open = {4, 9, 12};
    coroutine<pud_unfold, void> unfolding = unfold_loop.drive_unfold_loop();
    EXPECT_CALL(expander, expand(4, 0));
    unfolding.resume();
    ::testing::Mock::VerifyAndClearExpectations(&expander);
    EXPECT_CALL(expander, expand(9, 0));
    unfolding.resume();
}

TEST_F(PudUnfoldLoopTest, LeavesOpenedByAnExpansionAreOfferedNext) {
    open = {4};
    ON_CALL(expander, expand(4, _)).WillByDefault([this](pud_node_id, pud_body_goal_id) {
        open.erase(4);
        open.insert({20, 21});
        return std::vector<pud_node_id>{20, 21};
    });
    coroutine<pud_unfold, void> unfolding = unfold_loop.drive_unfold_loop();
    unfolding.resume();
    unfolding.consume_yield();
    EXPECT_CALL(next_unfolds, get_next_unfold(std::set<pud_node_id>{20, 21})).WillOnce(Return(pud_unfold{21, 0}));
    unfolding.resume();
}

TEST_F(PudUnfoldLoopTest, SearchEndsOnceTheLastOpenLeafIsExpanded) {
    open = {4, 9};
    coroutine<pud_unfold, void> unfolding = unfold_loop.drive_unfold_loop();
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
    ON_CALL(expander, expand(_, _)).WillByDefault(Return(std::vector<pud_node_id>{}));
    coroutine<pud_unfold, void> unfolding = unfold_loop.drive_unfold_loop();
    for (int round = 0; round < 3; ++round) {
        unfolding.resume();
        ASSERT_TRUE(unfolding.has_yield());
        EXPECT_EQ(unfolding.consume_yield(), (pud_unfold{4, 0}));
    }
}
