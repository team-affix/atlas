// dbuct_nearest_decision maps each resolution to its nearest ancestor decision.
// A decision resolution is its own nearest decision; a unit resolution inherits
// the nearest decision of its grandparent resolution (rl->parent->parent). The
// map is seeded with {nullptr -> nullptr} so root-level lookups resolve to "no
// decision". Every mutation is journaled on the trail so it can be backtracked.
//
// SUT: dbuct_nearest_decision (real). Collaborator: trail log (mocked).

#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/dbuct_nearest_decision.hpp"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::NiceMock;
using ::testing::StrictMock;

struct MockTrail {
    MOCK_METHOD(void, log, (std::unique_ptr<i_backtrackable>));
};

struct DbuctNearestDecisionTest : public ::testing::Test {
    NiceMock<MockTrail> trail;
    dbuct_nearest_decision<NiceMock<MockTrail>> nd{trail};
};

TEST_F(DbuctNearestDecisionTest, NearestOfNullIsNull) {
    EXPECT_EQ(nd.get_nearest_decision(nullptr), nullptr);
}

TEST_F(DbuctNearestDecisionTest, DecisionIsItsOwnNearest) {
    resolution_lineage d{nullptr, 0};

    nd.note_decision_resolution(&d);

    EXPECT_EQ(nd.get_nearest_decision(&d), &d);
}

TEST_F(DbuctNearestDecisionTest, UnitInheritsGrandparentNearestDecision) {
    resolution_lineage d{nullptr, 0};
    goal_lineage gd{&d, 0};
    resolution_lineage u{&gd, 0};

    nd.note_decision_resolution(&d);
    nd.note_unit_resolution(&u);

    EXPECT_EQ(nd.get_nearest_decision(&u), &d);
}

TEST_F(DbuctNearestDecisionTest, UnitChainPropagatesNearestDecision) {
    resolution_lineage d{nullptr, 0};
    goal_lineage gd{&d, 0};
    resolution_lineage u1{&gd, 0};
    goal_lineage g1{&u1, 0};
    resolution_lineage u2{&g1, 0};

    nd.note_decision_resolution(&d);
    nd.note_unit_resolution(&u1);
    nd.note_unit_resolution(&u2);

    EXPECT_EQ(nd.get_nearest_decision(&u1), &d);
    EXPECT_EQ(nd.get_nearest_decision(&u2), &d);
}

TEST_F(DbuctNearestDecisionTest, RootUnitInheritsNullNearest) {
    goal_lineage g{nullptr, 0};
    resolution_lineage u{&g, 0};

    nd.note_unit_resolution(&u);

    EXPECT_EQ(nd.get_nearest_decision(&u), nullptr);
}

TEST_F(DbuctNearestDecisionTest, NoteUnitWithUnrecordedGrandparentThrows) {
    resolution_lineage unrecorded{nullptr, 7};
    goal_lineage g{&unrecorded, 0};
    resolution_lineage u{&g, 0};

    // Precondition: the grandparent resolution must already be recorded; looking
    // up an unknown key is an out-of-range error rather than a silent bad insert.
    EXPECT_THROW(nd.note_unit_resolution(&u), std::out_of_range);
}

TEST_F(DbuctNearestDecisionTest, NoteJournalsBacktrackable) {
    StrictMock<MockTrail> strict_trail;
    dbuct_nearest_decision<StrictMock<MockTrail>> strict_nd{strict_trail};
    resolution_lineage d{nullptr, 0};

    // Contract: recording a resolution must journal at least one undo action so a
    // later pop can remove it. The exact number of entries is an implementation
    // detail, so this asserts AtLeast(1), not an exact count.
    EXPECT_CALL(strict_trail, log(_)).Times(AtLeast(1));
    strict_nd.note_decision_resolution(&d);
}
