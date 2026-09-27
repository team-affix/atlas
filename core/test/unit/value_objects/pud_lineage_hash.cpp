// pud_lineage_hash: hashes pud_lineage variants for unordered containers.

#include <gtest/gtest.h>
#include <unordered_set>
#include "value_objects/pud_lineage_hash.hpp"

struct PudLineageHashTest : public ::testing::Test {
    pud_lineage_hash hasher;

    pud_lineage axiom0{pud_lineage::axiom{0}};
    pud_lineage axiom1{pud_lineage::axiom{1}};
};

TEST_F(PudLineageHashTest, SameAxiomHashesEqual) {
    const pud_lineage axiom0_copy{pud_lineage::axiom{0}};
    EXPECT_EQ(hasher(axiom0), hasher(axiom0_copy));
}

TEST_F(PudLineageHashTest, DifferentAxiomEntryIdxHashesDiffer) {
    EXPECT_NE(hasher(axiom0), hasher(axiom1));
}

TEST_F(PudLineageHashTest, AxiomAndInferenceAlternativesTaggedDistinctly) {
    const pud_lineage inf{pud_lineage::inference{&axiom0, 0, &axiom1}};
    EXPECT_NE(hasher(axiom0), hasher(inf));
}

TEST_F(PudLineageHashTest, SameInferenceHashesEqual) {
    const pud_lineage inf0{pud_lineage::inference{&axiom0, 2, &axiom1}};
    const pud_lineage inf0_copy{pud_lineage::inference{&axiom0, 2, &axiom1}};
    EXPECT_EQ(hasher(inf0), hasher(inf0_copy));
}

TEST_F(PudLineageHashTest, DifferentCallSiteHashesDiffer) {
    const pud_lineage inf0{pud_lineage::inference{&axiom0, 0, &axiom1}};
    const pud_lineage inf1{pud_lineage::inference{&axiom0, 1, &axiom1}};
    EXPECT_NE(hasher(inf0), hasher(inf1));
}

TEST_F(PudLineageHashTest, DifferentCallerPointerHashesDiffer) {
    const pud_lineage inf0{pud_lineage::inference{&axiom0, 0, &axiom1}};
    const pud_lineage inf1{pud_lineage::inference{&axiom1, 0, &axiom1}};
    EXPECT_NE(hasher(inf0), hasher(inf1));
}

TEST_F(PudLineageHashTest, DifferentCalleePointerHashesDiffer) {
    const pud_lineage inf0{pud_lineage::inference{&axiom0, 0, &axiom1}};
    const pud_lineage inf1{pud_lineage::inference{&axiom0, 0, &axiom0}};
    EXPECT_NE(hasher(inf0), hasher(inf1));
}

TEST_F(PudLineageHashTest, WorksAsUnorderedSetKey) {
    const pud_lineage inf0{pud_lineage::inference{&axiom0, 0, &axiom1}};
    const pud_lineage inf1{pud_lineage::inference{&axiom0, 1, &axiom1}};
    std::unordered_set<pud_lineage, pud_lineage_hash> keys;

    keys.insert(axiom0);
    keys.insert(inf0);
    keys.insert(inf1);

    EXPECT_EQ(keys.size(), 3u);
    EXPECT_TRUE(keys.contains(axiom0));
    EXPECT_TRUE(keys.contains(inf0));
    EXPECT_TRUE(keys.contains(inf1));
}

TEST_F(PudLineageHashTest, DuplicateAxiomDoesNotGrowUnorderedSet) {
    std::unordered_set<pud_lineage, pud_lineage_hash> keys;
    keys.insert(axiom0);
    keys.insert(pud_lineage{pud_lineage::axiom{0}});
    EXPECT_EQ(keys.size(), 1u);
}

TEST_F(PudLineageHashTest, DuplicateInferenceDoesNotGrowUnorderedSet) {
    const pud_lineage inf{pud_lineage::inference{&axiom0, 0, &axiom1}};
    std::unordered_set<pud_lineage, pud_lineage_hash> keys;
    keys.insert(inf);
    keys.insert(pud_lineage{pud_lineage::inference{&axiom0, 0, &axiom1}});
    EXPECT_EQ(keys.size(), 1u);
}

TEST_F(PudLineageHashTest, HashUsesCallerPointerIdentityNotAxiomValue) {
    pud_lineage caller_a{pud_lineage::axiom{0}};
    pud_lineage caller_b{pud_lineage::axiom{0}};
    const pud_lineage inf_a{pud_lineage::inference{&caller_a, 0, &axiom1}};
    const pud_lineage inf_b{pud_lineage::inference{&caller_b, 0, &axiom1}};
    EXPECT_NE(inf_a, inf_b);
    EXPECT_NE(hasher(inf_a), hasher(inf_b));
}

TEST_F(PudLineageHashTest, HashUsesCalleePointerIdentityNotAxiomValue) {
    pud_lineage callee_a{pud_lineage::axiom{1}};
    pud_lineage callee_b{pud_lineage::axiom{1}};
    const pud_lineage inf_a{pud_lineage::inference{&axiom0, 0, &callee_a}};
    const pud_lineage inf_b{pud_lineage::inference{&axiom0, 0, &callee_b}};
    EXPECT_NE(inf_a, inf_b);
    EXPECT_NE(hasher(inf_a), hasher(inf_b));
}

TEST_F(PudLineageHashTest, SelfCallHashesEqualAcrossCopies) {
    const pud_lineage self_call{pud_lineage::inference{&axiom0, 0, &axiom0}};
    const pud_lineage self_call_copy{pud_lineage::inference{&axiom0, 0, &axiom0}};
    EXPECT_EQ(hasher(self_call), hasher(self_call_copy));
}

TEST_F(PudLineageHashTest, NestedCallerPointerAffectsHash) {
    const pud_lineage nested{pud_lineage::inference{&axiom0, 0, &axiom1}};
    const pud_lineage from_nested{pud_lineage::inference{&nested, 0, &axiom0}};
    const pud_lineage from_axiom{pud_lineage::inference{&axiom0, 0, &axiom0}};
    EXPECT_NE(hasher(from_nested), hasher(from_axiom));
}

TEST_F(PudLineageHashTest, AxiomZeroTaggedApartFromInferenceCallSiteZero) {
    const pud_lineage inf{pud_lineage::inference{&axiom0, 0, &axiom0}};
    EXPECT_NE(hasher(axiom0), hasher(inf));
}
