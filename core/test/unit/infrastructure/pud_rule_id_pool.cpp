// pud_rule_id_pool canonicalizes pud_rule_id nodes in a persistent set. Unit
// tests cover intern identity and DAG sharing without external mocks.

#include <gtest/gtest.h>
#include <stdexcept>
#include <variant>
#include "infrastructure/pud_rule_id_pool.hpp"

struct PudRuleIdPoolTest : public ::testing::Test {
    pud_rule_id_pool pool;
};

// ---------------------------------------------------------------------------
// Interning / identity
// ---------------------------------------------------------------------------

TEST_F(PudRuleIdPoolTest, AxiomInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make_axiom(0), pool.make_axiom(0));
}

TEST_F(PudRuleIdPoolTest, DifferentAxiomEntryIdxReturnDifferentPointers) {
    EXPECT_NE(pool.make_axiom(0), pool.make_axiom(1));
}

TEST_F(PudRuleIdPoolTest, InferenceInternedTwiceReturnsSamePointer) {
    const pud_rule_id* caller = pool.make_axiom(0);
    const pud_rule_id* callee = pool.make_axiom(1);
    EXPECT_EQ(pool.make_inference(caller, 0, callee), pool.make_inference(caller, 0, callee));
}

TEST_F(PudRuleIdPoolTest, DifferentCallersReturnDifferentPointers) {
    const pud_rule_id* caller0 = pool.make_axiom(0);
    const pud_rule_id* caller1 = pool.make_axiom(1);
    const pud_rule_id* callee = pool.make_axiom(2);
    EXPECT_NE(pool.make_inference(caller0, 0, callee), pool.make_inference(caller1, 0, callee));
}

TEST_F(PudRuleIdPoolTest, DifferentCallSitesReturnDifferentPointers) {
    const pud_rule_id* caller = pool.make_axiom(0);
    const pud_rule_id* callee = pool.make_axiom(1);
    EXPECT_NE(pool.make_inference(caller, 0, callee), pool.make_inference(caller, 1, callee));
}

TEST_F(PudRuleIdPoolTest, DifferentCalleesReturnDifferentPointers) {
    const pud_rule_id* caller = pool.make_axiom(0);
    const pud_rule_id* callee0 = pool.make_axiom(1);
    const pud_rule_id* callee1 = pool.make_axiom(2);
    EXPECT_NE(pool.make_inference(caller, 0, callee0), pool.make_inference(caller, 0, callee1));
}

TEST_F(PudRuleIdPoolTest, AxiomAndInferenceReturnDifferentPointers) {
    const pud_rule_id* axiom = pool.make_axiom(0);
    const pud_rule_id* callee = pool.make_axiom(1);
    EXPECT_NE(axiom, pool.make_inference(axiom, 0, callee));
}

TEST_F(PudRuleIdPoolTest, NullCallerThrows) {
    const pud_rule_id* callee = pool.make_axiom(0);
    EXPECT_THROW(pool.make_inference(nullptr, 0, callee), std::logic_error);
}

TEST_F(PudRuleIdPoolTest, NullCalleeThrows) {
    const pud_rule_id* caller = pool.make_axiom(0);
    EXPECT_THROW(pool.make_inference(caller, 0, nullptr), std::logic_error);
}

// ---------------------------------------------------------------------------
// DAG sharing
// ---------------------------------------------------------------------------

TEST_F(PudRuleIdPoolTest, InferencesShareCallerAndCalleePointers) {
    const pud_rule_id* caller = pool.make_axiom(0);
    const pud_rule_id* callee = pool.make_axiom(1);
    const pud_rule_id* inf0 = pool.make_inference(caller, 0, callee);
    const pud_rule_id* inf1 = pool.make_inference(caller, 1, callee);

    const pud_rule_id::inference& inf0_body = std::get<pud_rule_id::inference>(inf0->content);
    const pud_rule_id::inference& inf1_body = std::get<pud_rule_id::inference>(inf1->content);
    EXPECT_EQ(inf0_body.caller, caller);
    EXPECT_EQ(inf1_body.caller, caller);
    EXPECT_EQ(inf0_body.callee, callee);
    EXPECT_EQ(inf1_body.callee, callee);
    EXPECT_NE(inf0, inf1);
}

TEST_F(PudRuleIdPoolTest, InferenceCanUseAnotherInferenceAsCaller) {
    const pud_rule_id* axiom0 = pool.make_axiom(0);
    const pud_rule_id* axiom1 = pool.make_axiom(1);
    const pud_rule_id* inf0 = pool.make_inference(axiom0, 0, axiom1);
    const pud_rule_id* inf1 = pool.make_inference(inf0, 1, axiom0);

    const pud_rule_id::inference& inf1_body = std::get<pud_rule_id::inference>(inf1->content);
    EXPECT_EQ(inf1_body.caller, inf0);
    EXPECT_EQ(inf1_body.callee, axiom0);
}

TEST_F(PudRuleIdPoolTest, RepeatedInternOfTheSameTripleDoesNotCreateANewId) {
    const pud_rule_id* axiom0 = pool.make_axiom(0);
    pool.make_axiom(0);
    const pud_rule_id* axiom1 = pool.make_axiom(1);
    const pud_rule_id* inf = pool.make_inference(axiom0, 0, axiom1);
    EXPECT_EQ(pool.make_inference(axiom0, 0, axiom1), inf);
}

TEST_F(PudRuleIdPoolTest, InternedAxiomHoldsEntryIdx) {
    const pud_rule_id* axiom = pool.make_axiom(7);
    ASSERT_TRUE(std::holds_alternative<pud_rule_id::axiom>(axiom->content));
    EXPECT_EQ(std::get<pud_rule_id::axiom>(axiom->content).entry_idx, 7u);
}

TEST_F(PudRuleIdPoolTest, InternedInferenceHoldsCallTriple) {
    const pud_rule_id* caller = pool.make_axiom(0);
    const pud_rule_id* callee = pool.make_axiom(1);
    const pud_rule_id* inf = pool.make_inference(caller, 4, callee);
    ASSERT_TRUE(std::holds_alternative<pud_rule_id::inference>(inf->content));
    const pud_rule_id::inference& body = std::get<pud_rule_id::inference>(inf->content);
    EXPECT_EQ(body.caller, caller);
    EXPECT_EQ(body.call_site, 4u);
    EXPECT_EQ(body.callee, callee);
}

TEST_F(PudRuleIdPoolTest, SelfCallInternedTwiceReturnsSamePointer) {
    const pud_rule_id* axiom = pool.make_axiom(0);
    EXPECT_EQ(pool.make_inference(axiom, 0, axiom), pool.make_inference(axiom, 0, axiom));
}

TEST_F(PudRuleIdPoolTest, DistinctCallSitesWithSelfCallReturnDifferentPointers) {
    const pud_rule_id* axiom = pool.make_axiom(0);
    EXPECT_NE(pool.make_inference(axiom, 0, axiom), pool.make_inference(axiom, 1, axiom));
}

TEST_F(PudRuleIdPoolTest, InferenceAsCalleeInternedTwiceReturnsSamePointer) {
    const pud_rule_id* axiom0 = pool.make_axiom(0);
    const pud_rule_id* axiom1 = pool.make_axiom(1);
    const pud_rule_id* callee_inf = pool.make_inference(axiom0, 0, axiom1);
    EXPECT_EQ(pool.make_inference(axiom0, 1, callee_inf),
              pool.make_inference(axiom0, 1, callee_inf));
}

TEST_F(PudRuleIdPoolTest, InferenceAsCallerAndCalleeInternedTwiceReturnsSamePointer) {
    const pud_rule_id* axiom0 = pool.make_axiom(0);
    const pud_rule_id* axiom1 = pool.make_axiom(1);
    const pud_rule_id* inf = pool.make_inference(axiom0, 0, axiom1);
    EXPECT_EQ(pool.make_inference(inf, 2, inf), pool.make_inference(inf, 2, inf));
}

TEST_F(PudRuleIdPoolTest, NestedChainInternedTwiceReturnsSamePointer) {
    const pud_rule_id* axiom0 = pool.make_axiom(0);
    const pud_rule_id* axiom1 = pool.make_axiom(1);
    const pud_rule_id* inf0 = pool.make_inference(axiom0, 0, axiom1);
    const pud_rule_id* inf1 = pool.make_inference(inf0, 1, axiom0);
    const pud_rule_id* inf2 = pool.make_inference(inf1, 0, inf0);
    EXPECT_EQ(pool.make_inference(inf1, 0, inf0), inf2);
}

TEST_F(PudRuleIdPoolTest, SharedCalleeAcrossDifferentCallers) {
    const pud_rule_id* caller0 = pool.make_axiom(0);
    const pud_rule_id* caller1 = pool.make_axiom(1);
    const pud_rule_id* callee = pool.make_axiom(2);
    const pud_rule_id* inf0 = pool.make_inference(caller0, 0, callee);
    const pud_rule_id* inf1 = pool.make_inference(caller1, 0, callee);
    EXPECT_EQ(std::get<pud_rule_id::inference>(inf0->content).callee,
              std::get<pud_rule_id::inference>(inf1->content).callee);
    EXPECT_NE(inf0, inf1);
}

TEST_F(PudRuleIdPoolTest, ThreeLevelDagSharesIntermediateInference) {
    const pud_rule_id* axiom0 = pool.make_axiom(0);
    const pud_rule_id* axiom1 = pool.make_axiom(1);
    const pud_rule_id* mid = pool.make_inference(axiom0, 0, axiom1);
    const pud_rule_id* leaf0 = pool.make_inference(mid, 0, axiom0);
    const pud_rule_id* leaf1 = pool.make_inference(mid, 1, axiom1);
    EXPECT_EQ(std::get<pud_rule_id::inference>(leaf0->content).caller, mid);
    EXPECT_EQ(std::get<pud_rule_id::inference>(leaf1->content).caller, mid);
    EXPECT_NE(leaf0, leaf1);
}

TEST_F(PudRuleIdPoolTest, DistinctPoolsDoNotSharePointers) {
    pud_rule_id_pool other;
    EXPECT_NE(pool.make_axiom(0), other.make_axiom(0));
    const pud_rule_id* caller = pool.make_axiom(0);
    const pud_rule_id* callee = pool.make_axiom(1);
    const pud_rule_id* other_caller = other.make_axiom(0);
    const pud_rule_id* other_callee = other.make_axiom(1);
    EXPECT_NE(pool.make_inference(caller, 0, callee),
              other.make_inference(other_caller, 0, other_callee));
}

TEST_F(PudRuleIdPoolTest, ReinternAfterOtherInsertsReturnsSamePointer) {
    const pud_rule_id* axiom0 = pool.make_axiom(0);
    const pud_rule_id* axiom1 = pool.make_axiom(1);
    const pud_rule_id* inf = pool.make_inference(axiom0, 0, axiom1);
    pool.make_axiom(2);
    pool.make_axiom(3);
    pool.make_inference(axiom1, 1, axiom0);
    EXPECT_EQ(pool.make_axiom(0), axiom0);
    EXPECT_EQ(pool.make_inference(axiom0, 0, axiom1), inf);
}

TEST_F(PudRuleIdPoolTest, NullCallerDoesNotIntern) {
    const pud_rule_id* callee = pool.make_axiom(0);
    EXPECT_THROW(pool.make_inference(nullptr, 0, callee), std::logic_error);
    EXPECT_EQ(pool.make_axiom(0), callee);
}

TEST_F(PudRuleIdPoolTest, NullCalleeDoesNotIntern) {
    const pud_rule_id* caller = pool.make_axiom(0);
    EXPECT_THROW(pool.make_inference(caller, 0, nullptr), std::logic_error);
    EXPECT_EQ(pool.make_axiom(0), caller);
}

TEST_F(PudRuleIdPoolTest, BothNullCallerAndCalleeThrows) {
    EXPECT_THROW(pool.make_inference(nullptr, 0, nullptr), std::logic_error);
}

TEST_F(PudRuleIdPoolTest, LargeEntryIdxInterns) {
    const pud_rule_id* axiom = pool.make_axiom(1000000);
    EXPECT_EQ(pool.make_axiom(1000000), axiom);
    EXPECT_EQ(std::get<pud_rule_id::axiom>(axiom->content).entry_idx, 1000000u);
}

TEST_F(PudRuleIdPoolTest, LargeCallSiteInterns) {
    const pud_rule_id* caller = pool.make_axiom(0);
    const pud_rule_id* callee = pool.make_axiom(1);
    const pud_rule_id* inf = pool.make_inference(caller, 1000000, callee);
    EXPECT_EQ(pool.make_inference(caller, 1000000, callee), inf);
    EXPECT_EQ(std::get<pud_rule_id::inference>(inf->content).call_site, 1000000u);
}
