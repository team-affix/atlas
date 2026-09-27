// pud_lineage: the defaulted operator<=> on axiom vs inference alternatives.
//
// Interned pointers are the identity used as map keys, so an ordering that
// conflated two of them would merge distinct unfold paths.

#include <array>
#include <gtest/gtest.h>
#include <variant>
#include "value_objects/pud_lineage.hpp"

struct PudLineageTest : public ::testing::Test {
    // Held in one array so the two axiom pointers have a defined relative order.
    std::array<pud_lineage, 2> axioms{
        pud_lineage{pud_lineage::axiom{0}}, pud_lineage{pud_lineage::axiom{1}}};
};

TEST_F(PudLineageTest, AxiomHoldsEntryIdx) {
    EXPECT_EQ(std::get<pud_lineage::axiom>(axioms[0].content).entry_idx, 0u);
    EXPECT_EQ(std::get<pud_lineage::axiom>(axioms[1].content).entry_idx, 1u);
}

TEST_F(PudLineageTest, AxiomsOrderByEntryIdx) {
    EXPECT_EQ(axioms[0], (pud_lineage{pud_lineage::axiom{0}}));
    EXPECT_NE(axioms[0], axioms[1]);
    EXPECT_LT(axioms[0], axioms[1]);
    EXPECT_GT(axioms[1], axioms[0]);
}

TEST_F(PudLineageTest, DistantAxiomEntryIdxStillOrdersByEntryIdx) {
    const pud_lineage axiom2{pud_lineage::axiom{2}};
    EXPECT_LT(axioms[1], axiom2);
    EXPECT_LT(axioms[0], axiom2);
}

TEST_F(PudLineageTest, AxiomAndInferenceAreDistinct) {
    const pud_lineage inf{pud_lineage::inference{&axioms[0], 0, &axioms[1]}};
    EXPECT_NE(axioms[0], inf);
    EXPECT_LT(axioms[0], inf);
    EXPECT_TRUE(std::holds_alternative<pud_lineage::axiom>(axioms[0].content));
    EXPECT_TRUE(std::holds_alternative<pud_lineage::inference>(inf.content));
}

TEST_F(PudLineageTest, InferenceHoldsCallerCallSiteCallee) {
    const pud_lineage inf{pud_lineage::inference{&axioms[0], 3, &axioms[1]}};
    const pud_lineage::inference& body = std::get<pud_lineage::inference>(inf.content);
    EXPECT_EQ(body.caller, &axioms[0]);
    EXPECT_EQ(body.call_site, 3u);
    EXPECT_EQ(body.callee, &axioms[1]);
}

TEST_F(PudLineageTest, EqualInferencesCompareEqual) {
    const pud_lineage inf{pud_lineage::inference{&axioms[0], 0, &axioms[1]}};
    const pud_lineage inf_copy{pud_lineage::inference{&axioms[0], 0, &axioms[1]}};
    EXPECT_EQ(inf, inf_copy);
}

TEST_F(PudLineageTest, InferencesOrderByCallerThenCallSiteThenCallee) {
    const pud_lineage same_caller_call_site_0{pud_lineage::inference{&axioms[0], 0, &axioms[1]}};
    const pud_lineage same_caller_call_site_1{pud_lineage::inference{&axioms[0], 1, &axioms[1]}};
    EXPECT_EQ(same_caller_call_site_0, (pud_lineage{pud_lineage::inference{&axioms[0], 0, &axioms[1]}}));
    EXPECT_NE(same_caller_call_site_0, same_caller_call_site_1);
    EXPECT_LT(same_caller_call_site_0, same_caller_call_site_1);

    const pud_lineage same_caller_other_callee{pud_lineage::inference{&axioms[0], 0, &axioms[0]}};
    EXPECT_NE(same_caller_call_site_0, same_caller_other_callee);

    const pud_lineage other_caller{pud_lineage::inference{&axioms[1], 0, &axioms[0]}};
    EXPECT_NE(same_caller_call_site_0, other_caller);
    EXPECT_LT(same_caller_call_site_0, other_caller);
    EXPECT_LT(same_caller_call_site_1, other_caller);
}

TEST_F(PudLineageTest, CallSiteDominatesCallee) {
    // Smaller call_site wins even when the other inference has the earlier callee pointer.
    const pud_lineage earlier_site_later_callee{pud_lineage::inference{&axioms[0], 0, &axioms[1]}};
    const pud_lineage later_site_earlier_callee{pud_lineage::inference{&axioms[0], 1, &axioms[0]}};
    EXPECT_LT(earlier_site_later_callee, later_site_earlier_callee);
}

TEST_F(PudLineageTest, CalleeBreaksTiesWhenCallerAndCallSiteMatch) {
    const pud_lineage callee0{pud_lineage::inference{&axioms[0], 0, &axioms[0]}};
    const pud_lineage callee1{pud_lineage::inference{&axioms[0], 0, &axioms[1]}};
    EXPECT_NE(callee0, callee1);
    EXPECT_LT(callee0, callee1);
}

TEST_F(PudLineageTest, NestedInferenceCallerDiffersFromAxiomCaller) {
    const pud_lineage nested_caller{pud_lineage::inference{&axioms[0], 0, &axioms[1]}};
    const pud_lineage from_nested{pud_lineage::inference{&nested_caller, 0, &axioms[0]}};
    const pud_lineage from_axiom{pud_lineage::inference{&axioms[0], 0, &axioms[0]}};
    EXPECT_NE(from_nested, from_axiom);
}

TEST_F(PudLineageTest, SelfCallIsDistinctFromCrossCall) {
    const pud_lineage self_call{pud_lineage::inference{&axioms[0], 0, &axioms[0]}};
    const pud_lineage cross_call{pud_lineage::inference{&axioms[0], 0, &axioms[1]}};
    EXPECT_NE(self_call, cross_call);
}
