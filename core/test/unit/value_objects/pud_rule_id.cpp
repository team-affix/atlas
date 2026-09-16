// pud_rule_id: the defaulted operator<=> on axiom vs inference alternatives.
//
// Interned pointers are the identity used as map keys, so an ordering that
// conflated two of them would merge distinct unfold paths.

#include <array>
#include <gtest/gtest.h>
#include <variant>
#include "value_objects/pud_rule_id.hpp"

struct PudRuleIdTest : public ::testing::Test {
    // Held in one array so the two axiom pointers have a defined relative order.
    std::array<pud_rule_id, 2> axioms{
        pud_rule_id{pud_rule_id::axiom{0}}, pud_rule_id{pud_rule_id::axiom{1}}};
};

TEST_F(PudRuleIdTest, AxiomHoldsEntryIdx) {
    EXPECT_EQ(std::get<pud_rule_id::axiom>(axioms[0].content).entry_idx, 0u);
    EXPECT_EQ(std::get<pud_rule_id::axiom>(axioms[1].content).entry_idx, 1u);
}

TEST_F(PudRuleIdTest, AxiomsOrderByEntryIdx) {
    EXPECT_EQ(axioms[0], (pud_rule_id{pud_rule_id::axiom{0}}));
    EXPECT_NE(axioms[0], axioms[1]);
    EXPECT_LT(axioms[0], axioms[1]);
    EXPECT_GT(axioms[1], axioms[0]);
}

TEST_F(PudRuleIdTest, DistantAxiomEntryIdxStillOrdersByEntryIdx) {
    const pud_rule_id axiom2{pud_rule_id::axiom{2}};
    EXPECT_LT(axioms[1], axiom2);
    EXPECT_LT(axioms[0], axiom2);
}

TEST_F(PudRuleIdTest, AxiomAndInferenceAreDistinct) {
    const pud_rule_id inf{pud_rule_id::inference{&axioms[0], 0, &axioms[1]}};
    EXPECT_NE(axioms[0], inf);
    EXPECT_LT(axioms[0], inf);
    EXPECT_TRUE(std::holds_alternative<pud_rule_id::axiom>(axioms[0].content));
    EXPECT_TRUE(std::holds_alternative<pud_rule_id::inference>(inf.content));
}

TEST_F(PudRuleIdTest, InferenceHoldsCallerCallSiteCallee) {
    const pud_rule_id inf{pud_rule_id::inference{&axioms[0], 3, &axioms[1]}};
    const pud_rule_id::inference& body = std::get<pud_rule_id::inference>(inf.content);
    EXPECT_EQ(body.caller, &axioms[0]);
    EXPECT_EQ(body.call_site, 3u);
    EXPECT_EQ(body.callee, &axioms[1]);
}

TEST_F(PudRuleIdTest, EqualInferencesCompareEqual) {
    const pud_rule_id inf{pud_rule_id::inference{&axioms[0], 0, &axioms[1]}};
    const pud_rule_id inf_copy{pud_rule_id::inference{&axioms[0], 0, &axioms[1]}};
    EXPECT_EQ(inf, inf_copy);
}

TEST_F(PudRuleIdTest, InferencesOrderByCallerThenCallSiteThenCallee) {
    const pud_rule_id same_caller_call_site_0{pud_rule_id::inference{&axioms[0], 0, &axioms[1]}};
    const pud_rule_id same_caller_call_site_1{pud_rule_id::inference{&axioms[0], 1, &axioms[1]}};
    EXPECT_EQ(same_caller_call_site_0, (pud_rule_id{pud_rule_id::inference{&axioms[0], 0, &axioms[1]}}));
    EXPECT_NE(same_caller_call_site_0, same_caller_call_site_1);
    EXPECT_LT(same_caller_call_site_0, same_caller_call_site_1);

    const pud_rule_id same_caller_other_callee{pud_rule_id::inference{&axioms[0], 0, &axioms[0]}};
    EXPECT_NE(same_caller_call_site_0, same_caller_other_callee);

    const pud_rule_id other_caller{pud_rule_id::inference{&axioms[1], 0, &axioms[0]}};
    EXPECT_NE(same_caller_call_site_0, other_caller);
    EXPECT_LT(same_caller_call_site_0, other_caller);
    EXPECT_LT(same_caller_call_site_1, other_caller);
}

TEST_F(PudRuleIdTest, CallSiteDominatesCallee) {
    // Smaller call_site wins even when the other inference has the earlier callee pointer.
    const pud_rule_id earlier_site_later_callee{pud_rule_id::inference{&axioms[0], 0, &axioms[1]}};
    const pud_rule_id later_site_earlier_callee{pud_rule_id::inference{&axioms[0], 1, &axioms[0]}};
    EXPECT_LT(earlier_site_later_callee, later_site_earlier_callee);
}

TEST_F(PudRuleIdTest, CalleeBreaksTiesWhenCallerAndCallSiteMatch) {
    const pud_rule_id callee0{pud_rule_id::inference{&axioms[0], 0, &axioms[0]}};
    const pud_rule_id callee1{pud_rule_id::inference{&axioms[0], 0, &axioms[1]}};
    EXPECT_NE(callee0, callee1);
    EXPECT_LT(callee0, callee1);
}

TEST_F(PudRuleIdTest, NestedInferenceCallerDiffersFromAxiomCaller) {
    const pud_rule_id nested_caller{pud_rule_id::inference{&axioms[0], 0, &axioms[1]}};
    const pud_rule_id from_nested{pud_rule_id::inference{&nested_caller, 0, &axioms[0]}};
    const pud_rule_id from_axiom{pud_rule_id::inference{&axioms[0], 0, &axioms[0]}};
    EXPECT_NE(from_nested, from_axiom);
}

TEST_F(PudRuleIdTest, SelfCallIsDistinctFromCrossCall) {
    const pud_rule_id self_call{pud_rule_id::inference{&axioms[0], 0, &axioms[0]}};
    const pud_rule_id cross_call{pud_rule_id::inference{&axioms[0], 0, &axioms[1]}};
    EXPECT_NE(self_call, cross_call);
}
