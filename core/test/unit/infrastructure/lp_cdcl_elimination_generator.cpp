// lp_cdcl_elimination_generator caches avoidances per decision frame and pushes
// new ones down one edge at a time. The store has no collaborators, so these are
// pure behavioral tests driven through learn / descend / constrain / cleanup.
//
// decide() and unit() mirror the two paths run_sim takes per resolution: a
// decision descends into the child frame and then constrains, a unit resolution
// only constrains.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/lp_cdcl_elimination_generator.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace {

std::vector<const resolution_lineage*> collect_elims(
    coroutine<const resolution_lineage*, void> sm) {
    std::vector<const resolution_lineage*> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            out.push_back(sm.consume_yield());
    }
    return out;
}

} // namespace

struct LpCdclEliminationGeneratorTest : public ::testing::Test {
protected:
    lp_cdcl_elimination_generator cdcl;

    // A decision: descend into the child frame, then constrain as run_sim does.
    std::vector<const resolution_lineage*> decide(const resolution_lineage* rl) {
        cdcl.descend(rl);
        return collect_elims(cdcl.constrain(rl));
    }

    // A unit resolution: constrain only, no frame change.
    std::vector<const resolution_lineage*> unit(const resolution_lineage* rl) {
        return collect_elims(cdcl.constrain(rl));
    }

    // Walk a decision path, learn the avoidance over it, and end the sim.
    void learn_over(std::initializer_list<const resolution_lineage*> decisions) {
        for (const resolution_lineage* rl : decisions)
            cdcl.descend(rl);
        cdcl.learn();
        cdcl.cleanup();
    }

    goal_lineage g0{nullptr, 0};
    goal_lineage g1{nullptr, 1};
    goal_lineage g2{nullptr, 2};
    goal_lineage g3{nullptr, 3};

    resolution_lineage g0_r0{&g0, 0};
    resolution_lineage g0_r9{&g0, 9};
    resolution_lineage g1_r1{&g1, 1};
    resolution_lineage g1_r9{&g1, 9};
    resolution_lineage g2_r2{&g2, 2};
    resolution_lineage g3_r3{&g3, 3};

    // A three-deep chain: g00 is a subgoal of g0_r0, g000 a subgoal of g00_r1.
    // This is the shape a lemma would collapse to just {g000_r2}.
    goal_lineage g00{&g0_r0, 0};
    resolution_lineage g00_r1{&g00, 1};
    goal_lineage g000{&g00_r1, 0};
    resolution_lineage g000_r2{&g000, 2};
};

TEST_F(LpCdclEliminationGeneratorTest, LearnWithNoDecisionsReturnsNothing) {
    EXPECT_EQ(cdcl.learn(), std::nullopt);
}

TEST_F(LpCdclEliminationGeneratorTest, LearnSingleDecisionReturnsItAsElimination) {
    cdcl.descend(&g0_r0);
    EXPECT_EQ(cdcl.learn(), std::optional{&g0_r0});
}

TEST_F(LpCdclEliminationGeneratorTest, LearnMultipleDecisionsStoresAndReturnsNothing) {
    cdcl.descend(&g0_r0);
    cdcl.descend(&g1_r1);
    EXPECT_EQ(cdcl.learn(), std::nullopt);
}

TEST_F(LpCdclEliminationGeneratorTest, LearnUsesFullDecisionSetNotJustTheLeaf) {
    // If only the leaf were stored, descending the first decision would find no
    // member to reduce and the pair below would never become unit.
    learn_over({&g0_r0, &g00_r1, &g000_r2});
    EXPECT_THAT(decide(&g0_r0), IsEmpty());
    EXPECT_THAT(decide(&g00_r1), ElementsAre(&g000_r2));
}

TEST_F(LpCdclEliminationGeneratorTest, AncestorTakingAnotherRuleTrimsTheWholeSubtree) {
    // The satisfaction a lemma-based avoidance could never detect: g0 resolves
    // differently, so nothing below it can ever complete the avoidance.
    learn_over({&g0_r0, &g00_r1, &g000_r2});
    EXPECT_THAT(decide(&g0_r9), IsEmpty());
    EXPECT_THAT(decide(&g00_r1), IsEmpty());
    EXPECT_THAT(decide(&g000_r2), IsEmpty());
}

TEST_F(LpCdclEliminationGeneratorTest, DescendReducesOnMatchingMember) {
    learn_over({&g0_r0, &g1_r1, &g2_r2});
    EXPECT_THAT(decide(&g0_r0), IsEmpty());
    EXPECT_THAT(decide(&g1_r1), ElementsAre(&g2_r2));
}

TEST_F(LpCdclEliminationGeneratorTest, DescendTrimsOnSiblingResolution) {
    learn_over({&g0_r0, &g1_r1, &g2_r2});
    EXPECT_THAT(decide(&g0_r9), IsEmpty());
    EXPECT_THAT(decide(&g1_r1), IsEmpty());
}

TEST_F(LpCdclEliminationGeneratorTest, UnitOnDescentIsEmittedAndNotCached) {
    learn_over({&g0_r0, &g1_r1});
    EXPECT_THAT(decide(&g0_r0), ElementsAre(&g1_r1));
    EXPECT_THAT(decide(&g2_r2), IsEmpty());
}

TEST_F(LpCdclEliminationGeneratorTest, ContinuationStopsRedeliveryAcrossSims) {
    learn_over({&g0_r0, &g1_r1, &g2_r2});
    EXPECT_THAT(decide(&g0_r0), IsEmpty());
    cdcl.cleanup();
    // Same edge again: the child already has everything the root holds.
    EXPECT_THAT(decide(&g0_r0), IsEmpty());
    // The cached copy survived the sim boundary, already reduced by g0_r0.
    EXPECT_THAT(decide(&g1_r1), ElementsAre(&g2_r2));
}

TEST_F(LpCdclEliminationGeneratorTest, ForcedEliminationIsReemittedOnEveryVisit) {
    // Eliminations are undone at tear-down, so a frame that already proved one
    // has to re-apply it every time the sim walks back into it.
    learn_over({&g0_r0, &g1_r1, &g2_r2});
    EXPECT_THAT(decide(&g0_r0), IsEmpty());
    EXPECT_THAT(decide(&g1_r1), ElementsAre(&g2_r2));
    cdcl.cleanup();
    EXPECT_THAT(decide(&g0_r0), IsEmpty());
    EXPECT_THAT(decide(&g1_r1), ElementsAre(&g2_r2));
}

TEST_F(LpCdclEliminationGeneratorTest, TwoParentEdgesIntoOneChildDedupById) {
    learn_over({&g0_r0, &g1_r1, &g2_r2, &g3_r3});
    EXPECT_THAT(decide(&g0_r0), IsEmpty());
    EXPECT_THAT(decide(&g1_r1), IsEmpty());
    cdcl.cleanup();
    // Reach the same frame through the other order; the redelivery is dropped.
    EXPECT_THAT(decide(&g1_r1), IsEmpty());
    EXPECT_THAT(decide(&g0_r0), IsEmpty());
    // One cached copy, so one elimination rather than two.
    EXPECT_THAT(decide(&g2_r2), ElementsAre(&g3_r3));
}

TEST_F(LpCdclEliminationGeneratorTest, LocalConstrainReducesOnUnitResolution) {
    learn_over({&g0_r0, &g1_r1, &g2_r2});
    EXPECT_THAT(decide(&g0_r0), IsEmpty());
    EXPECT_THAT(unit(&g1_r1), ElementsAre(&g2_r2));
}

TEST_F(LpCdclEliminationGeneratorTest, LocalConstrainTombstonesOnSatisfaction) {
    learn_over({&g0_r0, &g1_r1, &g2_r2});
    EXPECT_THAT(decide(&g0_r0), IsEmpty());
    EXPECT_THAT(unit(&g1_r9), IsEmpty());
    EXPECT_THAT(unit(&g2_r2), IsEmpty());
}

TEST_F(LpCdclEliminationGeneratorTest, LocalConstrainReducesStepwiseAtRoot) {
    learn_over({&g0_r0, &g1_r1, &g2_r2});
    EXPECT_THAT(unit(&g0_r0), IsEmpty());
    EXPECT_THAT(unit(&g1_r1), ElementsAre(&g2_r2));
}

TEST_F(LpCdclEliminationGeneratorTest, TombstonedEntryIsNotPropagated) {
    learn_over({&g0_r0, &g1_r1});
    EXPECT_THAT(unit(&g0_r9), IsEmpty());
    cdcl.cleanup();
    EXPECT_THAT(decide(&g0_r0), IsEmpty());
}

TEST_F(LpCdclEliminationGeneratorTest, ConstrainOnUnrelatedGoalYieldsNothing) {
    learn_over({&g0_r0, &g1_r1});
    EXPECT_THAT(unit(&g3_r3), IsEmpty());
}

TEST_F(LpCdclEliminationGeneratorTest, IndependentAvoidancesDoNotInterfere) {
    learn_over({&g0_r0, &g1_r1});
    learn_over({&g2_r2, &g3_r3});
    EXPECT_THAT(decide(&g0_r0), ElementsAre(&g1_r1));
    EXPECT_THAT(decide(&g2_r2), ElementsAre(&g3_r3));
}

TEST_F(LpCdclEliminationGeneratorTest, CleanupReturnsToRootWithoutDroppingCaches) {
    learn_over({&g0_r0, &g1_r1, &g2_r2});
    EXPECT_THAT(decide(&g0_r0), IsEmpty());
    cdcl.cleanup();
    // Back at the root frame: a sibling of the first decision still sees the
    // root cache and trims against it.
    EXPECT_THAT(decide(&g0_r9), IsEmpty());
    EXPECT_THAT(decide(&g1_r1), IsEmpty());
}
