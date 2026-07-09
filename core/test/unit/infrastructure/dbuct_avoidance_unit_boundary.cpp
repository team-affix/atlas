// dbuct_avoidance_unit_boundary tracks the two most recent decisions (ultimate,
// penultimate) and a "unit boundary" frame index that always lags exactly one
// decision behind. On each log_decision it consults the nearest-decision oracle:
// if the new decision extends the current ultimate's chain it overwrites the
// ultimate in place; otherwise it rotates, promoting the old ultimate to
// penultimate and publishing the old ultimate's frame index as the new boundary.
//
// SUT: dbuct_avoidance_unit_boundary (real, only real component here). The
// nearest-decision infrastructure is MOCKED so the rotate-vs-overwrite branch is
// driven deterministically; the trail and frame-count are mocked too.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/dbuct_avoidance_unit_boundary.hpp"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::StrictMock;

namespace {

struct MockTrail {
    MOCK_METHOD(void, log, (std::unique_ptr<i_backtrackable>));
};

struct MockGetNearestDecision {
    MOCK_METHOD(const resolution_lineage*, get_nearest_decision, (const resolution_lineage*), (const));
};

struct MockGetFrameCount {
    MOCK_METHOD(size_t, depth, (), (const));
};

struct DbuctAvoidanceUnitBoundaryTest : public ::testing::Test {
    NiceMock<MockTrail> trail;
    NiceMock<MockGetNearestDecision> nd;
    NiceMock<MockGetFrameCount> fc;

    dbuct_avoidance_unit_boundary<NiceMock<MockGetNearestDecision>,
                                  NiceMock<MockGetFrameCount>,
                                  NiceMock<MockTrail>>
        sut{nd, fc, trail};

    // Two decisions on distinct chains, each one goal deep so rl->parent->parent
    // is a valid grandparent resolution to hand the nearest-decision oracle.
    resolution_lineage gp1{nullptr, 1};
    goal_lineage g1{&gp1, 0};
    resolution_lineage rl1{&g1, 0};

    resolution_lineage gp2{nullptr, 2};
    goal_lineage g2{&gp2, 0};
    resolution_lineage rl2{&g2, 0};

    // A decision distinct from ultimate at every step: returning it forces the
    // rotate branch (ultimate != nearest-decision-above-rl).
    resolution_lineage sentinel{nullptr, 99};

    size_t current_depth = 0;

    void SetUp() override {
        ON_CALL(fc, depth()).WillByDefault(ReturnPointee(&current_depth));
        // Default: both grandparents resolve to the sentinel, so consecutive
        // decisions rotate. Overwrite tests override the relevant lookup.
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
    current_depth = 3;
    sut.log_decision(&rl1);

    // INVARIANT (part 1): one decision behind means after the very first decision
    // the boundary has nothing to lag onto yet, so it stays at 0.
    EXPECT_EQ(sut.get_unit_boundary(), 0u);
    EXPECT_EQ(sut.get_ultimate_decision(), &rl1);
}

TEST_F(DbuctAvoidanceUnitBoundaryTest, SecondDecisionSetsBoundaryToPriorDecisionFrame) {
    constexpr size_t d1 = 3;
    constexpr size_t d2 = 7;

    current_depth = d1;
    sut.log_decision(&rl1);

    current_depth = d2;
    sut.log_decision(&rl2);

    // INVARIANT (part 2): the boundary now equals the frame index recorded at the
    // PRIOR decision (rl1 @ d1), i.e. it lags exactly one decision behind and is
    // NOT the current decision's frame (d2).
    EXPECT_EQ(sut.get_unit_boundary(), d1);
    EXPECT_NE(sut.get_unit_boundary(), d2);
    EXPECT_EQ(sut.get_penultimate_decision(), &rl1);
    EXPECT_EQ(sut.get_ultimate_decision(), &rl2);
}

TEST_F(DbuctAvoidanceUnitBoundaryTest, OverwriteWhenNewDecisionExtendsUltimateKeepsBoundary) {
    current_depth = 5;
    sut.log_decision(&rl1);

    // rl2 extends rl1's chain: the nearest decision above rl2 IS the current
    // ultimate (rl1), so the second decision overwrites in place instead of
    // rotating.
    ON_CALL(nd, get_nearest_decision(&gp2)).WillByDefault(Return(&rl1));

    current_depth = 9;
    sut.log_decision(&rl2);

    // Under a rotate this boundary would be 5 and penultimate would be &rl1; the
    // overwrite path must leave both untouched while advancing the ultimate.
    EXPECT_EQ(sut.get_unit_boundary(), 0u);
    EXPECT_EQ(sut.get_penultimate_decision(), nullptr);
    EXPECT_EQ(sut.get_ultimate_decision(), &rl2);
}

TEST_F(DbuctAvoidanceUnitBoundaryTest, LogDecisionJournalsBacktrackable) {
    StrictMock<MockTrail> strict_trail;
    dbuct_avoidance_unit_boundary<NiceMock<MockGetNearestDecision>,
                                  NiceMock<MockGetFrameCount>,
                                  StrictMock<MockTrail>>
        strict_sut{nd, fc, strict_trail};

    // Contract: the ultimate/boundary mutations must be journaled so a later pop
    // can undo them. The number of journal entries is an implementation detail,
    // hence AtLeast(1) rather than an exact count.
    EXPECT_CALL(strict_trail, log(_)).Times(AtLeast(1));

    current_depth = 4;
    strict_sut.log_decision(&rl1);
}

}  // namespace
