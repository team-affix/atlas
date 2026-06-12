// horizon_goal_deactivator: erases goal weight, then delegates to srt_goal_deactivator.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "locator_fixture.hpp"
#include "infrastructure/horizon_goal_deactivator.hpp"
#include "infrastructure/srt_goal_deactivator.hpp"
#include "interfaces/i_erase_goal_weight.hpp"
#include "interfaces/i_unset_goal_expr.hpp"
#include "interfaces/i_erase_goal_candidates.hpp"

struct MockEraseGoalWeight : public i_erase_goal_weight {
    MOCK_METHOD(void, erase, (const goal_lineage*), (override));
};

struct MockUnsetGoalExpr : public i_unset_goal_expr {
    MOCK_METHOD(void, unset, (const goal_lineage*), (override));
};

struct MockEraseGoalCandidates : public i_erase_goal_candidates {
    MOCK_METHOD(void, erase, (const goal_lineage*), (override));
};

struct MockSrtGoalDeactivator : public srt_goal_deactivator {
    explicit MockSrtGoalDeactivator(locator& loc) : srt_goal_deactivator(loc) {}
    MOCK_METHOD(void, deactivate, (const goal_lineage*), (override));
};

struct HorizonGoalDeactivatorTest : public ::testing::Test {
    locator loc;
    MockEraseGoalWeight erase_goal_weight;
    MockUnsetGoalExpr unset_goal_expr;
    MockEraseGoalCandidates erase_goal_candidates;
    std::unique_ptr<MockSrtGoalDeactivator> mock_srt;
    std::unique_ptr<horizon_goal_deactivator> deactivator;

    goal_lineage gl{nullptr, 0};

    void SetUp() override {
        loc.bind_as<i_erase_goal_weight>(erase_goal_weight);
        loc.bind_as<i_unset_goal_expr>(unset_goal_expr);
        loc.bind_as<i_erase_goal_candidates>(erase_goal_candidates);
        mock_srt = std::make_unique<MockSrtGoalDeactivator>(loc);
        loc.bind_as<srt_goal_deactivator>(*mock_srt);
        deactivator = std::make_unique<horizon_goal_deactivator>(loc);
    }
};

TEST_F(HorizonGoalDeactivatorTest, ErasesWeightThenDelegates) {
    testing::InSequence seq;
    EXPECT_CALL(erase_goal_weight, erase(&gl)).Times(1);
    EXPECT_CALL(*mock_srt, deactivate(&gl)).Times(1);
    deactivator->deactivate(&gl);
}
