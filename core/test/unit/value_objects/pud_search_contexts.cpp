// pud_witness_search_context, pud_witness_pair, and candidate counterparts: public data + <=>.

#include <gtest/gtest.h>
#include <optional>
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_pair.hpp"
#include "value_objects/pud_witness_search_context.hpp"

struct PudSearchValueObjectsTest : public ::testing::Test {
    PudSearchValueObjectsTest()
        : open_(1)
        , close_(2)
        , interval_{om_label(&open_), om_label(&close_)}
        , body_{expr::var{0}}
        , axiom_{pud_rule_id::axiom{0}} {}

    uint64_t open_;
    uint64_t close_;
    om_interval interval_;
    expr body_;
    pud_rule_id axiom_;
};

TEST_F(PudSearchValueObjectsTest, WitnessContextOrdersBySearchRootThenCurrent) {
    const pud_witness_search_context left{interval_, &body_, 0, &axiom_, &axiom_};
    const pud_witness_search_context right{interval_, &body_, 0, &axiom_, &axiom_};
    EXPECT_EQ(left, right);
}

TEST_F(PudSearchValueObjectsTest, WitnessPairOrdersByBothSides) {
    const pud_witness_search_context edge{interval_, &body_, 0, &axiom_, &axiom_};
    const pud_witness_pair left{edge, edge};
    const pud_witness_pair right{edge, edge};
    EXPECT_EQ(left, right);
}

TEST_F(PudSearchValueObjectsTest, ForcedUnfoldUnitAndRefutedAreDistinct) {
    const pud_forced_unfold unit{pud_forced_unfold::unit{&axiom_, 0}};
    const pud_forced_unfold refuted{pud_forced_unfold::refuted{&axiom_}};
    EXPECT_NE(unit, refuted);
}

TEST_F(PudSearchValueObjectsTest, CandidateContextHoldsOptionalWitnessPair) {
    const pud_witness_search_context edge{interval_, &body_, 0, &axiom_, &axiom_};
    pud_candidate_search_context ctx{
        interval_,
        &body_,
        0,
        &axiom_,
        pud_witness_pair{edge, edge},
        {}};
    ASSERT_TRUE(ctx.witnesses.has_value());
    EXPECT_EQ(ctx.witnesses->a.search_root, &axiom_);
    EXPECT_EQ(ctx.cursor, &axiom_);
}
