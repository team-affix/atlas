// pud_lineage_pool canonicalizes pud_lineage nodes in a persistent set. Unit
// tests cover intern identity and DAG sharing without external mocks.

#include <gtest/gtest.h>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <variant>
#include <vector>
#include "infrastructure/pud_lineage_pool.hpp"

struct PudLineagePoolTest : public ::testing::Test {
    pud_lineage_pool pool;
};

// ---------------------------------------------------------------------------
// Interning / identity
// ---------------------------------------------------------------------------

TEST_F(PudLineagePoolTest, AxiomInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make_axiom(0), pool.make_axiom(0));
}

TEST_F(PudLineagePoolTest, DifferentAxiomEntryIdxReturnDifferentPointers) {
    EXPECT_NE(pool.make_axiom(0), pool.make_axiom(1));
}

TEST_F(PudLineagePoolTest, InferenceInternedTwiceReturnsSamePointer) {
    const pud_lineage* caller = pool.make_axiom(0);
    const pud_lineage* callee = pool.make_axiom(1);
    EXPECT_EQ(pool.make_inference(caller, 0, callee), pool.make_inference(caller, 0, callee));
}

TEST_F(PudLineagePoolTest, DifferentCallersReturnDifferentPointers) {
    const pud_lineage* caller0 = pool.make_axiom(0);
    const pud_lineage* caller1 = pool.make_axiom(1);
    const pud_lineage* callee = pool.make_axiom(2);
    EXPECT_NE(pool.make_inference(caller0, 0, callee), pool.make_inference(caller1, 0, callee));
}

TEST_F(PudLineagePoolTest, DifferentCallSitesReturnDifferentPointers) {
    const pud_lineage* caller = pool.make_axiom(0);
    const pud_lineage* callee = pool.make_axiom(1);
    EXPECT_NE(pool.make_inference(caller, 0, callee), pool.make_inference(caller, 1, callee));
}

TEST_F(PudLineagePoolTest, DifferentCalleesReturnDifferentPointers) {
    const pud_lineage* caller = pool.make_axiom(0);
    const pud_lineage* callee0 = pool.make_axiom(1);
    const pud_lineage* callee1 = pool.make_axiom(2);
    EXPECT_NE(pool.make_inference(caller, 0, callee0), pool.make_inference(caller, 0, callee1));
}

TEST_F(PudLineagePoolTest, AxiomAndInferenceReturnDifferentPointers) {
    const pud_lineage* axiom = pool.make_axiom(0);
    const pud_lineage* callee = pool.make_axiom(1);
    EXPECT_NE(axiom, pool.make_inference(axiom, 0, callee));
}

TEST_F(PudLineagePoolTest, NullCallerThrows) {
    const pud_lineage* callee = pool.make_axiom(0);
    EXPECT_THROW(pool.make_inference(nullptr, 0, callee), std::logic_error);
}

TEST_F(PudLineagePoolTest, NullCalleeInternsAsQueryRoot) {
    const pud_lineage* caller = pool.make_axiom(0);
    const pud_lineage* callee = pool.make_axiom(1);
    const pud_lineage* query = pool.make_inference(caller, 0, nullptr);
    EXPECT_EQ(pool.make_inference(caller, 0, nullptr), query);
    EXPECT_NE(query, pool.make_inference(caller, 0, callee));
    EXPECT_EQ(std::get<pud_lineage::inference>(query->content).callee, nullptr);
}

TEST_F(PudLineagePoolTest, DistinctCallSitesWithNullCalleeReturnDifferentPointers) {
    const pud_lineage* caller = pool.make_axiom(0);
    EXPECT_NE(pool.make_inference(caller, 0, nullptr),
              pool.make_inference(caller, 1, nullptr));
}

// ---------------------------------------------------------------------------
// DAG sharing
// ---------------------------------------------------------------------------

TEST_F(PudLineagePoolTest, InferencesShareCallerAndCalleePointers) {
    const pud_lineage* caller = pool.make_axiom(0);
    const pud_lineage* callee = pool.make_axiom(1);
    const pud_lineage* inf0 = pool.make_inference(caller, 0, callee);
    const pud_lineage* inf1 = pool.make_inference(caller, 1, callee);

    const pud_lineage::inference& inf0_body = std::get<pud_lineage::inference>(inf0->content);
    const pud_lineage::inference& inf1_body = std::get<pud_lineage::inference>(inf1->content);
    EXPECT_EQ(inf0_body.caller, caller);
    EXPECT_EQ(inf1_body.caller, caller);
    EXPECT_EQ(inf0_body.callee, callee);
    EXPECT_EQ(inf1_body.callee, callee);
    EXPECT_NE(inf0, inf1);
}

TEST_F(PudLineagePoolTest, InferenceCanUseAnotherInferenceAsCaller) {
    const pud_lineage* axiom0 = pool.make_axiom(0);
    const pud_lineage* axiom1 = pool.make_axiom(1);
    const pud_lineage* inf0 = pool.make_inference(axiom0, 0, axiom1);
    const pud_lineage* inf1 = pool.make_inference(inf0, 1, axiom0);

    const pud_lineage::inference& inf1_body = std::get<pud_lineage::inference>(inf1->content);
    EXPECT_EQ(inf1_body.caller, inf0);
    EXPECT_EQ(inf1_body.callee, axiom0);
}

TEST_F(PudLineagePoolTest, RepeatedInternOfTheSameTripleDoesNotCreateANewId) {
    const pud_lineage* axiom0 = pool.make_axiom(0);
    pool.make_axiom(0);
    const pud_lineage* axiom1 = pool.make_axiom(1);
    const pud_lineage* inf = pool.make_inference(axiom0, 0, axiom1);
    EXPECT_EQ(pool.make_inference(axiom0, 0, axiom1), inf);
}

TEST_F(PudLineagePoolTest, InternedAxiomHoldsEntryIdx) {
    const pud_lineage* axiom = pool.make_axiom(7);
    ASSERT_TRUE(std::holds_alternative<pud_lineage::axiom>(axiom->content));
    EXPECT_EQ(std::get<pud_lineage::axiom>(axiom->content).entry_idx, 7u);
}

TEST_F(PudLineagePoolTest, InternedInferenceHoldsCallTriple) {
    const pud_lineage* caller = pool.make_axiom(0);
    const pud_lineage* callee = pool.make_axiom(1);
    const pud_lineage* inf = pool.make_inference(caller, 4, callee);
    ASSERT_TRUE(std::holds_alternative<pud_lineage::inference>(inf->content));
    const pud_lineage::inference& body = std::get<pud_lineage::inference>(inf->content);
    EXPECT_EQ(body.caller, caller);
    EXPECT_EQ(body.call_site, 4u);
    EXPECT_EQ(body.callee, callee);
}

TEST_F(PudLineagePoolTest, SelfCallInternedTwiceReturnsSamePointer) {
    const pud_lineage* axiom = pool.make_axiom(0);
    EXPECT_EQ(pool.make_inference(axiom, 0, axiom), pool.make_inference(axiom, 0, axiom));
}

TEST_F(PudLineagePoolTest, DistinctCallSitesWithSelfCallReturnDifferentPointers) {
    const pud_lineage* axiom = pool.make_axiom(0);
    EXPECT_NE(pool.make_inference(axiom, 0, axiom), pool.make_inference(axiom, 1, axiom));
}

TEST_F(PudLineagePoolTest, InferenceAsCalleeInternedTwiceReturnsSamePointer) {
    const pud_lineage* axiom0 = pool.make_axiom(0);
    const pud_lineage* axiom1 = pool.make_axiom(1);
    const pud_lineage* callee_inf = pool.make_inference(axiom0, 0, axiom1);
    EXPECT_EQ(pool.make_inference(axiom0, 1, callee_inf),
              pool.make_inference(axiom0, 1, callee_inf));
}

TEST_F(PudLineagePoolTest, InferenceAsCallerAndCalleeInternedTwiceReturnsSamePointer) {
    const pud_lineage* axiom0 = pool.make_axiom(0);
    const pud_lineage* axiom1 = pool.make_axiom(1);
    const pud_lineage* inf = pool.make_inference(axiom0, 0, axiom1);
    EXPECT_EQ(pool.make_inference(inf, 2, inf), pool.make_inference(inf, 2, inf));
}

TEST_F(PudLineagePoolTest, NestedChainInternedTwiceReturnsSamePointer) {
    const pud_lineage* axiom0 = pool.make_axiom(0);
    const pud_lineage* axiom1 = pool.make_axiom(1);
    const pud_lineage* inf0 = pool.make_inference(axiom0, 0, axiom1);
    const pud_lineage* inf1 = pool.make_inference(inf0, 1, axiom0);
    const pud_lineage* inf2 = pool.make_inference(inf1, 0, inf0);
    EXPECT_EQ(pool.make_inference(inf1, 0, inf0), inf2);
}

TEST_F(PudLineagePoolTest, SharedCalleeAcrossDifferentCallers) {
    const pud_lineage* caller0 = pool.make_axiom(0);
    const pud_lineage* caller1 = pool.make_axiom(1);
    const pud_lineage* callee = pool.make_axiom(2);
    const pud_lineage* inf0 = pool.make_inference(caller0, 0, callee);
    const pud_lineage* inf1 = pool.make_inference(caller1, 0, callee);
    EXPECT_EQ(std::get<pud_lineage::inference>(inf0->content).callee,
              std::get<pud_lineage::inference>(inf1->content).callee);
    EXPECT_NE(inf0, inf1);
}

TEST_F(PudLineagePoolTest, ThreeLevelDagSharesIntermediateInference) {
    const pud_lineage* axiom0 = pool.make_axiom(0);
    const pud_lineage* axiom1 = pool.make_axiom(1);
    const pud_lineage* mid = pool.make_inference(axiom0, 0, axiom1);
    const pud_lineage* leaf0 = pool.make_inference(mid, 0, axiom0);
    const pud_lineage* leaf1 = pool.make_inference(mid, 1, axiom1);
    EXPECT_EQ(std::get<pud_lineage::inference>(leaf0->content).caller, mid);
    EXPECT_EQ(std::get<pud_lineage::inference>(leaf1->content).caller, mid);
    EXPECT_NE(leaf0, leaf1);
}

TEST_F(PudLineagePoolTest, DistinctPoolsDoNotSharePointers) {
    pud_lineage_pool other;
    EXPECT_NE(pool.make_axiom(0), other.make_axiom(0));
    const pud_lineage* caller = pool.make_axiom(0);
    const pud_lineage* callee = pool.make_axiom(1);
    const pud_lineage* other_caller = other.make_axiom(0);
    const pud_lineage* other_callee = other.make_axiom(1);
    EXPECT_NE(pool.make_inference(caller, 0, callee),
              other.make_inference(other_caller, 0, other_callee));
}

TEST_F(PudLineagePoolTest, ReinternAfterOtherInsertsReturnsSamePointer) {
    const pud_lineage* axiom0 = pool.make_axiom(0);
    const pud_lineage* axiom1 = pool.make_axiom(1);
    const pud_lineage* inf = pool.make_inference(axiom0, 0, axiom1);
    pool.make_axiom(2);
    pool.make_axiom(3);
    pool.make_inference(axiom1, 1, axiom0);
    EXPECT_EQ(pool.make_axiom(0), axiom0);
    EXPECT_EQ(pool.make_inference(axiom0, 0, axiom1), inf);
}

TEST_F(PudLineagePoolTest, NullCallerDoesNotIntern) {
    const pud_lineage* callee = pool.make_axiom(0);
    EXPECT_THROW(pool.make_inference(nullptr, 0, callee), std::logic_error);
    EXPECT_EQ(pool.make_axiom(0), callee);
}

TEST_F(PudLineagePoolTest, NullCalleeInternDoesNotDisplaceCallerAxiom) {
    const pud_lineage* caller = pool.make_axiom(0);
    const pud_lineage* query = pool.make_inference(caller, 0, nullptr);
    EXPECT_EQ(pool.make_axiom(0), caller);
    EXPECT_EQ(pool.make_inference(caller, 0, nullptr), query);
}

TEST_F(PudLineagePoolTest, BothNullCallerAndCalleeThrows) {
    EXPECT_THROW(pool.make_inference(nullptr, 0, nullptr), std::logic_error);
}

TEST_F(PudLineagePoolTest, LargeEntryIdxInterns) {
    const pud_lineage* axiom = pool.make_axiom(1000000);
    EXPECT_EQ(pool.make_axiom(1000000), axiom);
    EXPECT_EQ(std::get<pud_lineage::axiom>(axiom->content).entry_idx, 1000000u);
}

TEST_F(PudLineagePoolTest, LargeCallSiteInterns) {
    const pud_lineage* caller = pool.make_axiom(0);
    const pud_lineage* callee = pool.make_axiom(1);
    const pud_lineage* inf = pool.make_inference(caller, 1000000, callee);
    EXPECT_EQ(pool.make_inference(caller, 1000000, callee), inf);
    EXPECT_EQ(std::get<pud_lineage::inference>(inf->content).call_site, 1000000u);
}

TEST_F(PudLineagePoolTest, ManyInternsPreserveEarlierPointers) {
    const pud_lineage* first_axiom = pool.make_axiom(0);
    const pud_lineage* first_callee = pool.make_axiom(1);
    const pud_lineage* first_inf = pool.make_inference(first_axiom, 0, first_callee);
    for (size_t idx = 2; idx < 202; ++idx)
        pool.make_axiom(idx);
    for (size_t idx = 0; idx < 200; ++idx)
        pool.make_inference(pool.make_axiom(idx), idx, pool.make_axiom(idx + 1));
    EXPECT_EQ(pool.make_axiom(0), first_axiom);
    EXPECT_EQ(pool.make_inference(first_axiom, 0, first_callee), first_inf);
}

TEST_F(PudLineagePoolTest, StressWideDagSharing) {
    std::vector<const pud_lineage*> callers;
    std::vector<const pud_lineage*> callees;
    callers.reserve(64);
    callees.reserve(8);
    for (size_t idx = 0; idx < 64; ++idx)
        callers.push_back(pool.make_axiom(idx));
    for (size_t idx = 0; idx < 8; ++idx)
        callees.push_back(pool.make_axiom(100 + idx));
    const pud_lineage* mid = pool.make_inference(callers[3], 2, callees[4]);
    for (size_t caller_idx = 0; caller_idx < 64; ++caller_idx) {
        for (size_t site = 0; site < 8; ++site) {
            for (size_t callee_idx = 0; callee_idx < 8; ++callee_idx)
                pool.make_inference(callers[caller_idx], site, callees[callee_idx]);
        }
    }
    EXPECT_EQ(pool.make_inference(callers[3], 2, callees[4]), mid);
}

TEST_F(PudLineagePoolTest, FuzzMakeAxiomAndInference) {
    constexpr uint32_t k_seed = 42;
    std::mt19937 rng{k_seed};
    std::uniform_int_distribution<int> op_dist(0, 1);
    std::ostringstream log;
    std::vector<const pud_lineage*> axioms;
    std::unordered_map<size_t, const pud_lineage*> axiom_by_idx;
    axioms.push_back(pool.make_axiom(0));
    axiom_by_idx[0] = axioms.back();
    for (int step = 0; step < 200; ++step) {
        const int op = op_dist(rng);
        log << step << ':' << op << ' ';
        if (op == 0 || axioms.size() < 2) {
            const size_t idx = axioms.size();
            const pud_lineage* id = pool.make_axiom(idx);
            axioms.push_back(id);
            axiom_by_idx[idx] = id;
            EXPECT_EQ(pool.make_axiom(idx), id) << "seed " << k_seed << " log " << log.str();
            EXPECT_EQ(std::get<pud_lineage::axiom>(id->content).entry_idx, idx)
                << "seed " << k_seed << " log " << log.str();
            continue;
        }
        const pud_lineage* caller = axioms[rng() % axioms.size()];
        const pud_lineage* callee = axioms[rng() % axioms.size()];
        const size_t site = rng() % 4;
        const pud_lineage* inf = pool.make_inference(caller, site, callee);
        EXPECT_EQ(pool.make_inference(caller, site, callee), inf)
            << "seed " << k_seed << " log " << log.str();
        const pud_lineage::inference& body = std::get<pud_lineage::inference>(inf->content);
        EXPECT_EQ(body.caller, caller) << "seed " << k_seed << " log " << log.str();
        EXPECT_EQ(body.call_site, site) << "seed " << k_seed << " log " << log.str();
        EXPECT_EQ(body.callee, callee) << "seed " << k_seed << " log " << log.str();
    }
}
