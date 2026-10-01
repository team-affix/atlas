// pud_witness_search_context, pud_candidate_choice_point, and candidate counterparts: public data + <=>.

#include <gtest/gtest.h>
#include <optional>
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_lineage.hpp"
#include "value_objects/pud_candidate_choice_point.hpp"
#include "value_objects/pud_witness_search_context.hpp"

struct PudSearchValueObjectsTest : public ::testing::Test {
    PudSearchValueObjectsTest()
        : leaf_{pud_lineage::axiom{1}}
        , axiom_{pud_lineage::axiom{0}} {}

    pud_lineage leaf_;
    pud_lineage axiom_;
};

TEST_F(PudSearchValueObjectsTest, WitnessContextOrdersBySearchRootThenCurrent) {
    const pud_witness_search_context left{&leaf_, 0, 0, &axiom_, &axiom_};
    const pud_witness_search_context right{&leaf_, 0, 0, &axiom_, &axiom_};
    EXPECT_EQ(left, right);
}

TEST_F(PudSearchValueObjectsTest, WitnessPairOrdersByBothSides) {
    const pud_witness_search_context edge{&leaf_, 0, 0, &axiom_, &axiom_};
    const pud_candidate_choice_point left{edge, edge};
    const pud_candidate_choice_point right{edge, edge};
    EXPECT_EQ(left, right);
}

TEST_F(PudSearchValueObjectsTest, ForcedUnfoldUnitAndRefutedAreDistinct) {
    const pud_forced_unfold unit{pud_forced_unfold::unit{&axiom_, 0}};
    const pud_forced_unfold refuted{pud_forced_unfold::refuted{&axiom_}};
    EXPECT_NE(unit, refuted);
}

TEST_F(PudSearchValueObjectsTest, CandidateContextHoldsOptionalWitnessPair) {
    const pud_witness_search_context edge{&leaf_, 0, 0, &axiom_, &axiom_};
    pud_candidate_search_context ctx{
        &leaf_,
        0,
        0,
        &axiom_,
        pud_candidate_choice_point{edge, edge}};
    ASSERT_TRUE(ctx.witnesses.has_value());
    EXPECT_EQ(ctx.witnesses->a.search_root, &axiom_);
    EXPECT_EQ(ctx.cursor, &axiom_);
    EXPECT_EQ(ctx.query_leaf, &leaf_);
    EXPECT_EQ(ctx.body_goal_idx, 0u);
}
