#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "pud_datalog_oracle.hpp"
#include "pud_manifest_harness.hpp"

namespace {
std::unique_ptr<pud_manifest_harness> load_and_run(unsigned program_seed, uint32_t random_seed, bool recursive) {
    auto harness = std::make_unique<pud_manifest_harness>(random_seed, pud_harness_scope::every_open_leaf);
    load_datalog_axioms(*harness, generate_datalog(program_seed, recursive));
    harness->open_axioms();
    return harness;
}

bool contains_all(const std::set<const expr*>& bigger, const std::set<const expr*>& smaller) {
    return std::includes(bigger.begin(), bigger.end(), smaller.begin(), smaller.end());
}

}

TEST(PudManifestOracleIntegrationTest, FlatProgramsProveExactlyTheLeastModel) {
    size_t facts_checked = 0;
    for (unsigned seed = 0; seed < 150; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        auto harness = load_and_run(seed, seed, false);
        harness->unfold_until_closed(20000);
        ASSERT_TRUE(harness->open_leaves().empty()) << "a flat program must unfold until nothing is open";
        const std::set<const expr*> model = datalog_model_exprs(*harness, generate_datalog(seed, false));
        EXPECT_EQ(harness->proven_heads(), model);
        facts_checked += model.size();
    }
    EXPECT_GE(facts_checked, 1500u) << "the random corpus is too easy to be meaningful";
}

TEST(PudManifestOracleIntegrationTest, EveryUnfoldOrderProvesTheSameModel) {
    for (unsigned seed = 400; seed < 420; ++seed) {
        for (uint32_t random_seed = 0; random_seed < 8; ++random_seed) {
            SCOPED_TRACE("program " + std::to_string(seed) + " order " + std::to_string(random_seed));
            auto harness = load_and_run(seed, random_seed, false);
            harness->unfold_until_closed(20000);
            EXPECT_EQ(harness->proven_heads(), datalog_model_exprs(*harness, generate_datalog(seed, false)));
        }
    }
}

TEST(PudManifestOracleIntegrationTest, RuleTreesGrownByHandStillProveTheSameModel) {
    for (unsigned seed = 500; seed < 560; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        auto harness = load_and_run(seed, seed, false);
        for (pud_node_id axiom_root : harness->manifest.axioms_) {
            if (harness->refuted(axiom_root) || !harness->open(axiom_root))
                continue;
            harness->grow_along_open_goals(axiom_root, 2);
        }
        harness->check_all();
        harness->unfold_until_closed(20000);
        EXPECT_EQ(harness->proven_heads(), datalog_model_exprs(*harness, generate_datalog(seed, false)));
    }
}

TEST(PudManifestOracleIntegrationTest, DuplicateFactsLeaveTheModelUnchanged) {
    for (unsigned seed = 600; seed < 640; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, false);
        auto harness = std::make_unique<pud_manifest_harness>(seed, pud_harness_scope::every_open_leaf);
        load_datalog_axioms(*harness, program);
        for (const datalog_atom& fact : program.facts)
            harness->axiom(datalog_expr(*harness, fact), {}, 0);
        harness->open_axioms();
        harness->unfold_until_closed(20000);
        EXPECT_EQ(harness->proven_heads(), datalog_model_exprs(*harness, program));
        harness->check_all();
    }
}

TEST(PudManifestOracleIntegrationTest, RecursiveProgramsOnlyProveEntailedFacts) {
    size_t proven_checked = 0;
    for (unsigned seed = 700; seed < 800; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        auto harness = load_and_run(seed, seed, true);
        harness->unfold_while_small(30, 64);
        const std::set<const expr*> proven = harness->proven_heads();
        const std::set<const expr*> model = datalog_model_exprs(*harness, generate_datalog(seed, true));
        EXPECT_TRUE(contains_all(model, proven)) << "a proven head is not in the least model";
        proven_checked += proven.size();
    }
    EXPECT_GE(proven_checked, 500u);
}

TEST(PudManifestOracleIntegrationTest, ProvenFactsOnlyGrowAsUnfoldingContinues) {
    for (unsigned seed = 800; seed < 860; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        auto harness = load_and_run(seed, seed, true);
        std::set<const expr*> previous = harness->proven_heads();
        for (int round = 0; round < 3; ++round) {
            harness->unfold_while_small(10, 64);
            const std::set<const expr*> current = harness->proven_heads();
            EXPECT_TRUE(contains_all(current, previous)) << "a proven head was lost in round " << round;
            previous = current;
        }
    }
}

TEST(PudManifestOracleIntegrationTest, FactsAreProvenBeforeAnyUnfold) {
    for (unsigned seed = 900; seed < 920; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, true);
        auto harness = load_and_run(seed, seed, true);
        std::set<const expr*> facts;
        for (const datalog_atom& fact : program.facts)
            facts.insert(datalog_expr(*harness, fact));
        EXPECT_EQ(harness->proven_heads(), facts);
    }
}
