#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_frontier_change_collector.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockCheckNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_node_id), (const));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_node_id));
};

struct PudFrontierChangeCollectorTest : public ::testing::Test {
    NiceMock<MockCheckNodeRefuted> status;
    NiceMock<MockGetNodeGoals> goals;
    pud_frontier_change_collector<NiceMock<MockCheckNodeRefuted>, NiceMock<MockGetNodeGoals>> collector{status, goals};
    expr goal{expr::functor{1, {}}};
    pud_goal_list open_goals = pud_goal_list{.goals = {}, .next_id = 0}.adding({&goal});
    pud_goal_list no_goals{.goals = {}, .next_id = 1};

    void SetUp() override {
        ON_CALL(goals, get(1)).WillByDefault(ReturnRef(open_goals));
        ON_CALL(goals, get(2)).WillByDefault(ReturnRef(no_goals));
        ON_CALL(goals, get(3)).WillByDefault(ReturnRef(open_goals));
        ON_CALL(status, is_refuted(3)).WillByDefault(Return(true));
    }
};

}

TEST_F(PudFrontierChangeCollectorTest, ProducedLeavesAreOpenedOrProvenAndRefutedOnesAreLeftOut) {
    EXPECT_EQ(collector.collect({1, 2, 3}, {3, 9}), (pud_frontier_change{.opened = {1}, .proven = {2}, .refuted = {3, 9}}));
}
