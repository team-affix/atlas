// dbuct_avoidance_unit_boundary tracks the two most recent decisions (ultimate,
// penultimate) and a "unit boundary" frame index that always lags exactly one
// decision behind. On each log_decision it consults the nearest-decision oracle:
// if the new decision extends the current ultimate's chain it overwrites the
// ultimate in place; otherwise it rotates, promoting the old ultimate to
// penultimate and publishing the old ultimate's frame index as the new boundary.
//
// SUT: dbuct_avoidance_unit_boundary (real). The nearest-decision infrastructure
// and frame depth are mocked so the rotate-vs-overwrite branch is deterministic.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/dbuct_avoidance_unit_boundary.hpp"
#include "infrastructure/frame_depth_tracker.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnPointee;

namespace {

struct MockGetNearestDecision {
    MOCK_METHOD(const resolution_lineage*, get_nearest_decision, (const resolution_lineage*), (const));
};

struct DbuctAvoidanceUnitBoundaryTest : public ::testing::Test {
    NiceMock<MockGetNearestDecision> nd;
    frame_depth_tracker fc;

    dbuct_avoidance_unit_boundary<NiceMock<MockGetNearestDecision>, frame_depth_tracker>
        sut{nd, fc};

    resolution_lineage gp1{nullptr, 1};
    goal_lineage g1{&gp1, 0};
    resolution_lineage rl1{&g1, 0};

    resolution_lineage gp2{nullptr, 2};
    goal_lineage g2{&gp2, 0};
    resolution_lineage rl2{&g2, 0};

    resolution_lineage sentinel{nullptr, 99};

    void SetUp() override {
        sut.push_frame();
        ON_CALL(nd, get_nearest_decision(&gp1)).WillByDefault(Return(&sentinel));
        ON_CALL(nd, get_nearest_decision(&gp2)).WillByDefault(Return(&sentinel));
    }
};

TEST_F(DbuctAvoidanceUnitBoundaryTest, InitiallyBoundaryZeroAndDecisionsNull) {
    EXPECT_EQ(sut.get_unit_boundary(), 0u);
    EXPECT_EQ(sut.get_ultimate_decision(), nullptr);
    EXPECT_EQ(sut.get_penultimate_decision(), nullptr);
}

TEST_F(DbuctAvoidanceUnitBoundaryTest, FirstDecisionLeavesBoundaryAtZero) {
    fc.push();
    sut.log_decision(&rl1);

    EXPECT_EQ(sut.get_unit_boundary(), 0u);
    EXPECT_EQ(sut.get_ultimate_decision(), &rl1);
}

TEST_F(DbuctAvoidanceUnitBoundaryTest, SecondDecisionSetsBoundaryToPriorDecisionFrame) {
    constexpr size_t d1 = 3;
    constexpr size_t d2 = 7;

    fc.push();
    while (fc.depth() < d1) fc.push();
    sut.log_decision(&rl1);

    while (fc.depth() < d2) fc.push();
    sut.log_decision(&rl2);

    EXPECT_EQ(sut.get_unit_boundary(), d1);
    EXPECT_NE(sut.get_unit_boundary(), d2);
    EXPECT_EQ(sut.get_penultimate_decision(), &rl1);
    EXPECT_EQ(sut.get_ultimate_decision(), &rl2);
}

TEST_F(DbuctAvoidanceUnitBoundaryTest, OverwriteWhenNewDecisionExtendsUltimateKeepsBoundary) {
    fc.push();
    sut.log_decision(&rl1);

    ON_CALL(nd, get_nearest_decision(&gp2)).WillByDefault(Return(&rl1));

    while (fc.depth() < 9) fc.push();
    sut.log_decision(&rl2);

    EXPECT_EQ(sut.get_unit_boundary(), 0u);
    EXPECT_EQ(sut.get_penultimate_decision(), nullptr);
    EXPECT_EQ(sut.get_ultimate_decision(), &rl2);
}

TEST_F(DbuctAvoidanceUnitBoundaryTest, UltimateDecisionDepthTracksLoggedFrame) {
    constexpr size_t d1 = 3;
    constexpr size_t d2 = 7;

    fc.push();
    while (fc.depth() < d1) fc.push();
    sut.log_decision(&rl1);
    EXPECT_EQ(sut.get_ultimate_decision_depth(), d1);

    while (fc.depth() < d2) fc.push();
    sut.log_decision(&rl2);
    EXPECT_EQ(sut.get_ultimate_decision_depth(), d2);
}

TEST_F(DbuctAvoidanceUnitBoundaryTest, PopRevertsLoggedDecision) {
    fc.push();
    sut.log_decision(&rl1);
    ASSERT_EQ(sut.get_ultimate_decision(), &rl1);
    sut.pop_frame();
    EXPECT_EQ(sut.get_ultimate_decision(), nullptr);
}

}  // namespace
