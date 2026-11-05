#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lazy_focus_frontier.hpp"
#include "pud_test_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockContainsFocusNode {
    MOCK_METHOD(bool, contains, (pud_global_node_id), (const));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id), (const));
};

struct PudLazyFocusFrontierTest : public ::testing::Test {
    NiceMock<MockContainsFocusNode> focus;
    NiceMock<MockGetNodeGoals> goals;
    pud_lazy_focus_frontier<NiceMock<MockContainsFocusNode>, NiceMock<MockGetNodeGoals>> frontier{focus, goals};
    expr goal{.content = expr::var{.index = 0}};
    pud_goal_list open_goals{.goals = immer::map<pud_body_goal_id, const expr*>{}.set(0, &goal), .next_id = 1};
    pud_goal_list no_goals{.goals = {}, .next_id = 1};

    void SetUp() override {
        ON_CALL(focus, contains(_)).WillByDefault(Return(true));
        ON_CALL(focus, contains(pud_test_node(9))).WillByDefault(Return(false));
        ON_CALL(goals, get(_)).WillByDefault(ReturnRef(open_goals));
        ON_CALL(goals, get(pud_test_node(4))).WillByDefault(ReturnRef(no_goals));
    }
};

}

TEST_F(PudLazyFocusFrontierTest, OnlyFocusOffspringBecomeLiveAndThoseWithoutGoalsAreSolved) {
    frontier.apply_register(
        pud_frontier_change{.offspring = {pud_test_node(3), pud_test_node(4), pud_test_node(9)}, .refuted = {}});
    EXPECT_THAT(frontier.live(), ElementsAre(pud_test_node(3), pud_test_node(4)));
    EXPECT_THAT(frontier.solved(), ElementsAre(pud_test_node(4)));
}

TEST_F(PudLazyFocusFrontierTest, AnUnfoldReplacesTheParentWithItsOffspringAndDropsTheRefuted) {
    frontier.apply_register(pud_frontier_change{.offspring = {pud_test_node(1), pud_test_node(2)}, .refuted = {}});
    frontier.apply_unfold(
        pud_test_node(1), pud_frontier_change{.offspring = {pud_test_node(3)}, .refuted = {pud_test_node(2)}});
    EXPECT_THAT(frontier.live(), ElementsAre(pud_test_node(3)));
}

TEST_F(PudLazyFocusFrontierTest, ErasedAndRefutedLeavesLeaveTheFrontier) {
    frontier.apply_register(
        pud_frontier_change{.offspring = {pud_test_node(2), pud_test_node(3), pud_test_node(4)}, .refuted = {}});
    frontier.erase(pud_test_node(4));
    frontier.erase_refuted({pud_test_node(2)});
    EXPECT_THAT(frontier.live(), ElementsAre(pud_test_node(3)));
    EXPECT_THAT(frontier.solved(), IsEmpty());
}
