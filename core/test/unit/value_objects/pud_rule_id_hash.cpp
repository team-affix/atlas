// pud_rule_id_hash: hashes pud_rule_id variants for unordered containers.

#include <gtest/gtest.h>
#include <unordered_set>
#include "value_objects/pud_rule_id_hash.hpp"

struct PudRuleIdHashTest : public ::testing::Test {
    pud_rule_id_hash hasher;

    pud_rule_id axiom0{pud_rule_id::axiom{0}};
    pud_rule_id axiom1{pud_rule_id::axiom{1}};
};

TEST_F(PudRuleIdHashTest, SameAxiomHashesEqual) {
    const pud_rule_id axiom0_copy{pud_rule_id::axiom{0}};
    EXPECT_EQ(hasher(axiom0), hasher(axiom0_copy));
}

TEST_F(PudRuleIdHashTest, DifferentAxiomEntryIdxHashesDiffer) {
    EXPECT_NE(hasher(axiom0), hasher(axiom1));
}

TEST_F(PudRuleIdHashTest, AxiomAndInferenceAlternativesTaggedDistinctly) {
    const pud_rule_id inf{pud_rule_id::inference{&axiom0, 0, &axiom1}};
    EXPECT_NE(hasher(axiom0), hasher(inf));
}

TEST_F(PudRuleIdHashTest, SameInferenceHashesEqual) {
    const pud_rule_id inf0{pud_rule_id::inference{&axiom0, 2, &axiom1}};
    const pud_rule_id inf0_copy{pud_rule_id::inference{&axiom0, 2, &axiom1}};
    EXPECT_EQ(hasher(inf0), hasher(inf0_copy));
}

TEST_F(PudRuleIdHashTest, DifferentCallSiteHashesDiffer) {
    const pud_rule_id inf0{pud_rule_id::inference{&axiom0, 0, &axiom1}};
    const pud_rule_id inf1{pud_rule_id::inference{&axiom0, 1, &axiom1}};
    EXPECT_NE(hasher(inf0), hasher(inf1));
}

TEST_F(PudRuleIdHashTest, DifferentCallerPointerHashesDiffer) {
    const pud_rule_id inf0{pud_rule_id::inference{&axiom0, 0, &axiom1}};
    const pud_rule_id inf1{pud_rule_id::inference{&axiom1, 0, &axiom1}};
    EXPECT_NE(hasher(inf0), hasher(inf1));
}

TEST_F(PudRuleIdHashTest, DifferentCalleePointerHashesDiffer) {
    const pud_rule_id inf0{pud_rule_id::inference{&axiom0, 0, &axiom1}};
    const pud_rule_id inf1{pud_rule_id::inference{&axiom0, 0, &axiom0}};
    EXPECT_NE(hasher(inf0), hasher(inf1));
}

TEST_F(PudRuleIdHashTest, WorksAsUnorderedSetKey) {
    const pud_rule_id inf0{pud_rule_id::inference{&axiom0, 0, &axiom1}};
    const pud_rule_id inf1{pud_rule_id::inference{&axiom0, 1, &axiom1}};
    std::unordered_set<pud_rule_id, pud_rule_id_hash> keys;

    keys.insert(axiom0);
    keys.insert(inf0);
    keys.insert(inf1);

    EXPECT_EQ(keys.size(), 3u);
    EXPECT_TRUE(keys.contains(axiom0));
    EXPECT_TRUE(keys.contains(inf0));
    EXPECT_TRUE(keys.contains(inf1));
}

TEST_F(PudRuleIdHashTest, DuplicateAxiomDoesNotGrowUnorderedSet) {
    std::unordered_set<pud_rule_id, pud_rule_id_hash> keys;
    keys.insert(axiom0);
    keys.insert(pud_rule_id{pud_rule_id::axiom{0}});
    EXPECT_EQ(keys.size(), 1u);
}

TEST_F(PudRuleIdHashTest, DuplicateInferenceDoesNotGrowUnorderedSet) {
    const pud_rule_id inf{pud_rule_id::inference{&axiom0, 0, &axiom1}};
    std::unordered_set<pud_rule_id, pud_rule_id_hash> keys;
    keys.insert(inf);
    keys.insert(pud_rule_id{pud_rule_id::inference{&axiom0, 0, &axiom1}});
    EXPECT_EQ(keys.size(), 1u);
}

TEST_F(PudRuleIdHashTest, HashUsesCallerPointerIdentityNotAxiomValue) {
    pud_rule_id caller_a{pud_rule_id::axiom{0}};
    pud_rule_id caller_b{pud_rule_id::axiom{0}};
    const pud_rule_id inf_a{pud_rule_id::inference{&caller_a, 0, &axiom1}};
    const pud_rule_id inf_b{pud_rule_id::inference{&caller_b, 0, &axiom1}};
    EXPECT_NE(inf_a, inf_b);
    EXPECT_NE(hasher(inf_a), hasher(inf_b));
}

TEST_F(PudRuleIdHashTest, HashUsesCalleePointerIdentityNotAxiomValue) {
    pud_rule_id callee_a{pud_rule_id::axiom{1}};
    pud_rule_id callee_b{pud_rule_id::axiom{1}};
    const pud_rule_id inf_a{pud_rule_id::inference{&axiom0, 0, &callee_a}};
    const pud_rule_id inf_b{pud_rule_id::inference{&axiom0, 0, &callee_b}};
    EXPECT_NE(inf_a, inf_b);
    EXPECT_NE(hasher(inf_a), hasher(inf_b));
}

TEST_F(PudRuleIdHashTest, SelfCallHashesEqualAcrossCopies) {
    const pud_rule_id self_call{pud_rule_id::inference{&axiom0, 0, &axiom0}};
    const pud_rule_id self_call_copy{pud_rule_id::inference{&axiom0, 0, &axiom0}};
    EXPECT_EQ(hasher(self_call), hasher(self_call_copy));
}

TEST_F(PudRuleIdHashTest, NestedCallerPointerAffectsHash) {
    const pud_rule_id nested{pud_rule_id::inference{&axiom0, 0, &axiom1}};
    const pud_rule_id from_nested{pud_rule_id::inference{&nested, 0, &axiom0}};
    const pud_rule_id from_axiom{pud_rule_id::inference{&axiom0, 0, &axiom0}};
    EXPECT_NE(hasher(from_nested), hasher(from_axiom));
}

TEST_F(PudRuleIdHashTest, AxiomZeroTaggedApartFromInferenceCallSiteZero) {
    const pud_rule_id inf{pud_rule_id::inference{&axiom0, 0, &axiom0}};
    EXPECT_NE(hasher(axiom0), hasher(inf));
}
