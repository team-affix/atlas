// dbuct_avoidance_unit_boundary tracks the two most recent decisions (ultimate,
// penultimate) and a penultimate choice depth that always lags exactly one
// decision behind. On each log_decision it consults the nearest-decision oracle:
// if the new decision extends the current ultimate's chain it overwrites the
// ultimate in place; otherwise it rotates, promoting the old ultimate to
// penultimate and publishing the old ultimate's choice depth as the new boundary.
//
// SUT: dbuct_avoidance_unit_boundary (real). The nearest-decision infrastructure
// and choice depth are mocked so the rotate-vs-overwrite branch is deterministic.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/dbuct_avoidance_unit_boundary.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetNearestDecision {
    MOCK_METHOD(const resolution_lineage*, get_nearest_decision, (const resolution_lineage*), (const));
};

struct MockGetChoiceDepth {
    MOCK_METHOD(size_t, depth, (), (const));
};

struct DbuctAvoidanceUnitBoundaryTest : public ::testing::Test {
    NiceMock<MockGetNearestDecision> nd;
    NiceMock<MockGetChoiceDepth> choice_depth;

    dbuct_avoidance_unit_boundary<NiceMock<MockGetNearestDecision>, MockGetChoiceDepth>
        sut{nd, choice_depth};

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
        ON_CALL(choice_depth, depth()).WillByDefault(Return(1u));
    }
};

TEST_F(DbuctAvoidanceUnitBoundaryTest, InitiallyBoundaryZeroAndDecisionsNull) {
    EXPECT_EQ(sut.get_penultimate_decision_choice_depth(), 0u);
    EXPECT_EQ(sut.get_ultimate_decision(), nullptr);
    EXPECT_EQ(sut.get_penultimate_decision(), nullptr);
}

TEST_F(DbuctAvoidanceUnitBoundaryTest, FirstDecisionLeavesBoundaryAtZero) {
    ON_CALL(choice_depth, depth()).WillByDefault(Return(2u));
    sut.log_decision(&rl1);

    EXPECT_EQ(sut.get_penultimate_decision_choice_depth(), 0u);
    EXPECT_EQ(sut.get_ultimate_decision(), &rl1);
}

TEST_F(DbuctAvoidanceUnitBoundaryTest, SecondDecisionSetsBoundaryToPriorDecisionChoiceDepth) {
    constexpr size_t d1 = 3;
    constexpr size_t d2 = 7;

    ON_CALL(choice_depth, depth()).WillByDefault(Return(d1));
    sut.log_decision(&rl1);

    ON_CALL(choice_depth, depth()).WillByDefault(Return(d2));
    sut.log_decision(&rl2);

    EXPECT_EQ(sut.get_penultimate_decision_choice_depth(), d1);
    EXPECT_NE(sut.get_penultimate_decision_choice_depth(), d2);
    EXPECT_EQ(sut.get_penultimate_decision(), &rl1);
    EXPECT_EQ(sut.get_ultimate_decision(), &rl2);
}

TEST_F(DbuctAvoidanceUnitBoundaryTest, OverwriteWhenNewDecisionExtendsUltimateKeepsBoundary) {
    ON_CALL(choice_depth, depth()).WillByDefault(Return(2u));
    sut.log_decision(&rl1);

    ON_CALL(nd, get_nearest_decision(&gp2)).WillByDefault(Return(&rl1));
    ON_CALL(choice_depth, depth()).WillByDefault(Return(9u));
    sut.log_decision(&rl2);

    EXPECT_EQ(sut.get_penultimate_decision_choice_depth(), 0u);
    EXPECT_EQ(sut.get_penultimate_decision(), nullptr);
    EXPECT_EQ(sut.get_ultimate_decision(), &rl2);
}

TEST_F(DbuctAvoidanceUnitBoundaryTest, UltimateDecisionChoiceDepthTracksLoggedChoiceDepth) {
    constexpr size_t d1 = 3;
    constexpr size_t d2 = 7;

    ON_CALL(choice_depth, depth()).WillByDefault(Return(d1));
    sut.log_decision(&rl1);
    EXPECT_EQ(sut.get_ultimate_decision_choice_depth(), d1);

    ON_CALL(choice_depth, depth()).WillByDefault(Return(d2));
    sut.log_decision(&rl2);
    EXPECT_EQ(sut.get_ultimate_decision_choice_depth(), d2);
}

TEST_F(DbuctAvoidanceUnitBoundaryTest, PopRevertsLoggedDecision) {
    ON_CALL(choice_depth, depth()).WillByDefault(Return(2u));
    sut.log_decision(&rl1);
    ASSERT_EQ(sut.get_ultimate_decision(), &rl1);
    sut.pop_frame();
    EXPECT_EQ(sut.get_ultimate_decision(), nullptr);
}

}  // namespace
