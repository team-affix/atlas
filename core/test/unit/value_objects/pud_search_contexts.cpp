// pud_witness_search_context / result and candidate counterparts: public data + <=>.

#include <gtest/gtest.h>
#include <variant>
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"

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

TEST_F(PudSearchValueObjectsTest, WitnessContextOrdersByEdgeThenCurrent) {
    const pud_witness_search_context left{interval_, &body_, 0, &axiom_, &axiom_};
    const pud_witness_search_context right{interval_, &body_, 0, &axiom_, &axiom_};
    EXPECT_EQ(left, right);
}

TEST_F(PudSearchValueObjectsTest, WitnessResultFoundDiffersFromFailed) {
    const pud_witness_search_result found{pud_witness_search_result::found{}};
    const pud_witness_search_result failed{pud_witness_search_result::failed{}};
    EXPECT_NE(found, failed);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(found.content));
}

TEST_F(PudSearchValueObjectsTest, CandidateResultAlternativesAreDistinct) {
    const pud_candidate_search_result choice{pud_candidate_search_result::choice_point{}};
    const pud_candidate_search_result self{pud_candidate_search_result::self_witness{}};
    const pud_candidate_search_result dead{pud_candidate_search_result::axiom_refuted{}};
    EXPECT_NE(choice, self);
    EXPECT_NE(self, dead);
}

TEST_F(PudSearchValueObjectsTest, ForcedUnfoldUnitAndRefutedAreDistinct) {
    const pud_forced_unfold unit{pud_forced_unfold::unit{&axiom_, 0}};
    const pud_forced_unfold refuted{pud_forced_unfold::refuted{&axiom_}};
    EXPECT_NE(unit, refuted);
}

TEST_F(PudSearchValueObjectsTest, CandidateContextHoldsLiveEdges) {
    pud_candidate_search_context ctx{
        interval_,
        &body_,
        0,
        &axiom_,
        {{interval_, &body_, 0, &axiom_, &axiom_}},
        {}};
    EXPECT_EQ(ctx.live_edges.size(), 1u);
    EXPECT_EQ(ctx.cursor, &axiom_);
}
