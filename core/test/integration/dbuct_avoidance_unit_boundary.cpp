// Integration slice: real dbuct_avoidance_unit_boundary wired to a real
// dbuct_nearest_decision. Choice depth uses a fake depth source. This proves the
// actual nearest-decision map selects the rotate-vs-overwrite branch, and that
// the penultimate choice depth lags exactly one decision behind end to end.

#include <gtest/gtest.h>
#include "infrastructure/dbuct_nearest_decision.hpp"
#include "infrastructure/dbuct_avoidance_unit_boundary.hpp"

namespace {

struct fake_choice_depth {
    size_t depth_value;
    size_t depth() const { return depth_value; }
};

struct DbuctAvoidanceUnitBoundaryIntegrationTest : public ::testing::Test {
    fake_choice_depth fc{1};
    dbuct_nearest_decision nd;
    dbuct_avoidance_unit_boundary<dbuct_nearest_decision, fake_choice_depth> aub{nd, fc};

    resolution_lineage R0{nullptr, 0};
    goal_lineage g0{&R0, 0};
    resolution_lineage Da{&g0, 0};
    goal_lineage g1{&Da, 0};
    resolution_lineage U{&g1, 0};
    goal_lineage g2{&U, 0};
    resolution_lineage Db{&g2, 0};
    goal_lineage g3{&R0, 1};
    resolution_lineage Dc{&g3, 0};

    void SetUp() override {
        nd.push_frame();
        aub.push_frame();
        nd.note_decision_resolution(&R0);
        nd.note_decision_resolution(&Da);
        nd.note_unit_resolution(&U);
        nd.note_decision_resolution(&Db);
        nd.note_decision_resolution(&Dc);
    }
};

TEST_F(DbuctAvoidanceUnitBoundaryIntegrationTest, FirstDecisionKeepsBoundaryZero) {
    fc.depth_value = 3;
    aub.log_decision(&Da);

    EXPECT_EQ(aub.get_penultimate_decision_choice_depth(), 0u);
    EXPECT_EQ(aub.get_ultimate_decision(), &Da);
}

TEST_F(DbuctAvoidanceUnitBoundaryIntegrationTest, OverwriteWhenSecondDecisionExtendsFirstChain) {
    fc.depth_value = 3;
    aub.log_decision(&Da);

    fc.depth_value = 7;
    aub.log_decision(&Db);

    EXPECT_EQ(aub.get_penultimate_decision_choice_depth(), 0u);
    EXPECT_EQ(aub.get_penultimate_decision(), nullptr);
    EXPECT_EQ(aub.get_ultimate_decision(), &Db);
}

TEST_F(DbuctAvoidanceUnitBoundaryIntegrationTest, RotateWhenSecondDecisionStartsNewChain) {
    fc.depth_value = 3;
    aub.log_decision(&Da);

    fc.depth_value = 9;
    aub.log_decision(&Dc);

    EXPECT_EQ(aub.get_penultimate_decision_choice_depth(), 3u);
    EXPECT_EQ(aub.get_penultimate_decision(), &Da);
    EXPECT_EQ(aub.get_ultimate_decision(), &Dc);
}

}  // namespace
