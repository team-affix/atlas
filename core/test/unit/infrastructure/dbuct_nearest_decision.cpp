// dbuct_nearest_decision maps each resolution to its nearest ancestor decision.
// A decision resolution is its own nearest decision; a unit resolution inherits
// the nearest decision of its grandparent resolution (rl->parent->parent). The
// map is seeded with {nullptr -> nullptr} so root-level lookups resolve to "no
// decision". Every mutation is journaled per frame so pop_frame can undo it.

#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/dbuct_nearest_decision.hpp"

struct DbuctNearestDecisionTest : public ::testing::Test {
    dbuct_nearest_decision nd;
};

TEST_F(DbuctNearestDecisionTest, NearestOfNullIsNull) {
    EXPECT_EQ(nd.get_nearest_decision(nullptr), nullptr);
}

TEST_F(DbuctNearestDecisionTest, DecisionIsItsOwnNearest) {
    resolution_lineage d{nullptr, 0};

    nd.push_frame();
    nd.note_decision_resolution(&d);

    EXPECT_EQ(nd.get_nearest_decision(&d), &d);
}

TEST_F(DbuctNearestDecisionTest, UnitInheritsGrandparentNearestDecision) {
    resolution_lineage d{nullptr, 0};
    goal_lineage gd{&d, 0};
    resolution_lineage u{&gd, 0};

    nd.push_frame();
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

    nd.push_frame();
    nd.note_decision_resolution(&d);
    nd.note_unit_resolution(&u1);
    nd.note_unit_resolution(&u2);

    EXPECT_EQ(nd.get_nearest_decision(&u1), &d);
    EXPECT_EQ(nd.get_nearest_decision(&u2), &d);
}

TEST_F(DbuctNearestDecisionTest, RootUnitInheritsNullNearest) {
    goal_lineage g{nullptr, 0};
    resolution_lineage u{&g, 0};

    nd.push_frame();
    nd.note_unit_resolution(&u);

    EXPECT_EQ(nd.get_nearest_decision(&u), nullptr);
}

TEST_F(DbuctNearestDecisionTest, NoteUnitWithUnrecordedGrandparentThrows) {
    resolution_lineage unrecorded{nullptr, 7};
    goal_lineage g{&unrecorded, 0};
    resolution_lineage u{&g, 0};

    nd.push_frame();
    EXPECT_THROW(nd.note_unit_resolution(&u), std::out_of_range);
}

TEST_F(DbuctNearestDecisionTest, PopRevertsDecisionInsert) {
    resolution_lineage d{nullptr, 0};

    nd.push_frame();
    nd.note_decision_resolution(&d);
    ASSERT_EQ(nd.get_nearest_decision(&d), &d);
    nd.pop_frame();
    EXPECT_THROW(nd.get_nearest_decision(&d), std::out_of_range);
}
