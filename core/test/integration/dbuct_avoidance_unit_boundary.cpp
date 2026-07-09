// Integration slice: real dbuct_avoidance_unit_boundary wired to a real
// dbuct_nearest_decision. Only these two production types are real; the trail and
// frame-count are mocked (they sit outside the slice). This proves the actual
// nearest-decision map -- not a stub -- selects the rotate-vs-overwrite branch,
// and that the unit boundary lags exactly one decision behind end to end.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/dbuct_nearest_decision.hpp"
#include "infrastructure/dbuct_avoidance_unit_boundary.hpp"

using ::testing::NiceMock;
using ::testing::ReturnPointee;

namespace {

struct MockTrail {
    MOCK_METHOD(void, log, (std::unique_ptr<i_backtrackable>));
};

struct MockGetFrameCount {
    MOCK_METHOD(size_t, depth, (), (const));
};

using nearest_decision_t = dbuct_nearest_decision<NiceMock<MockTrail>>;

struct DbuctAvoidanceUnitBoundaryIntegrationTest : public ::testing::Test {
    NiceMock<MockTrail> trail;
    NiceMock<MockGetFrameCount> fc;

    nearest_decision_t nd{trail};
    dbuct_avoidance_unit_boundary<nearest_decision_t, NiceMock<MockGetFrameCount>, NiceMock<MockTrail>>
        aub{nd, fc, trail};

    // Resolution tree:
    //   R0 (decision)
    //    |- Da (decision)          nearest-above == R0
    //    |    |- U (unit)          inherits Da
    //    |         |- Db (decision) nearest-above == Da  (extends Da's chain)
    //    |- Dc (decision)          nearest-above == R0  (new chain vs Da)
    resolution_lineage R0{nullptr, 0};
    goal_lineage g0{&R0, 0};
    resolution_lineage Da{&g0, 0};
    goal_lineage g1{&Da, 0};
    resolution_lineage U{&g1, 0};
    goal_lineage g2{&U, 0};
    resolution_lineage Db{&g2, 0};
    goal_lineage g3{&R0, 1};
    resolution_lineage Dc{&g3, 0};

    size_t current_depth = 0;

    void SetUp() override {
        ON_CALL(fc, depth()).WillByDefault(ReturnPointee(&current_depth));
        nd.note_decision_resolution(&R0);
        nd.note_decision_resolution(&Da);
        nd.note_unit_resolution(&U);
        nd.note_decision_resolution(&Db);
        nd.note_decision_resolution(&Dc);
    }
};

TEST_F(DbuctAvoidanceUnitBoundaryIntegrationTest, FirstDecisionKeepsBoundaryZero) {
    current_depth = 3;
    aub.log_decision(&Da);

    EXPECT_EQ(aub.get_unit_boundary(), 0u);
    EXPECT_EQ(aub.get_ultimate_decision(), &Da);
}

TEST_F(DbuctAvoidanceUnitBoundaryIntegrationTest, OverwriteWhenSecondDecisionExtendsFirstChain) {
    current_depth = 3;
    aub.log_decision(&Da);

    // Db's nearest ancestor decision (via the real nd) is Da == current ultimate,
    // so this overwrites in place: boundary and penultimate must not move.
    current_depth = 7;
    aub.log_decision(&Db);

    EXPECT_EQ(aub.get_unit_boundary(), 0u);
    EXPECT_EQ(aub.get_penultimate_decision(), nullptr);
    EXPECT_EQ(aub.get_ultimate_decision(), &Db);
}

TEST_F(DbuctAvoidanceUnitBoundaryIntegrationTest, RotateWhenSecondDecisionStartsNewChain) {
    current_depth = 3;
    aub.log_decision(&Da);

    // Dc's nearest ancestor decision (via the real nd) is R0 != current ultimate
    // Da, so this rotates: the boundary lags one decision behind, publishing Da's
    // frame index (3).
    current_depth = 9;
    aub.log_decision(&Dc);

    EXPECT_EQ(aub.get_unit_boundary(), 3u);
    EXPECT_EQ(aub.get_penultimate_decision(), &Da);
    EXPECT_EQ(aub.get_ultimate_decision(), &Dc);
}

}  // namespace
