// rp_fewer_candidates_elimination_router: on eliminated, refresh parent score.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/rp_fewer_candidates_elimination_router.hpp"
#include "value_objects/elimination_result.hpp"
#include "value_objects/lineage.hpp"

using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockRouteElimination {
    MOCK_METHOD(elimination_result, route, (const resolution_lineage*));
};

struct MockComputeActiveGoalValue {
    MOCK_METHOD(double, compute_active_goal_value, (const goal_lineage*));
};

struct MockSetActiveGoalValue {
    MOCK_METHOD(void, set_active_goal_value, (const goal_lineage*, double));
};

using router_t = rp_fewer_candidates_elimination_router<
    MockRouteElimination, MockComputeActiveGoalValue, MockSetActiveGoalValue>;

}  // namespace

struct RpFewerCandidatesEliminationRouterTest : public ::testing::Test {
    StrictMock<MockRouteElimination> route_elimination;
    StrictMock<MockComputeActiveGoalValue> compute;
    StrictMock<MockSetActiveGoalValue> set_value;
    router_t router{route_elimination, compute, set_value};

    goal_lineage parent{nullptr, 0};
    resolution_lineage rl{&parent, 3};
};

TEST_F(RpFewerCandidatesEliminationRouterTest, EliminatedRefreshesParentScore) {
    InSequence seq;
    EXPECT_CALL(route_elimination, route(&rl))
        .WillOnce(Return(elimination_result::eliminated));
    EXPECT_CALL(compute, compute_active_goal_value(&parent)).WillOnce(Return(-2.0));
    EXPECT_CALL(set_value, set_active_goal_value(&parent, -2.0));
    EXPECT_EQ(router.route(&rl), elimination_result::eliminated);
}

TEST_F(RpFewerCandidatesEliminationRouterTest, BacklogSkipsRefresh) {
    EXPECT_CALL(route_elimination, route(&rl))
        .WillOnce(Return(elimination_result::added_to_backlog));
    EXPECT_CALL(set_value, set_active_goal_value).Times(0);
    EXPECT_EQ(router.route(&rl), elimination_result::added_to_backlog);
}

TEST_F(RpFewerCandidatesEliminationRouterTest, AlreadyDeactivatedSkipsRefresh) {
    EXPECT_CALL(route_elimination, route(&rl))
        .WillOnce(Return(elimination_result::already_deactivated));
    EXPECT_CALL(set_value, set_active_goal_value).Times(0);
    EXPECT_EQ(router.route(&rl), elimination_result::already_deactivated);
}
