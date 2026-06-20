// cdcl_elimination_generator learns pairwise avoidance lemmas and yields eliminations
// during constrain. Unit tests exercise learn/constrain/cleanup through the public API only.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/cdcl_elimination_generator.hpp"
#include "infrastructure/chosen_goal_candidates.hpp"
#include "infrastructure/coroutine.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

namespace {

std::vector<const resolution_lineage*> collect_elims(
    coroutine<const resolution_lineage*, void> sm) {
    std::vector<const resolution_lineage*> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield()) {
            const resolution_lineage* v = sm.consume_yield();
            if (v != nullptr)
                out.push_back(v);
        }
    }
    return out;
}

lemma make_lemma(std::initializer_list<const resolution_lineage*> rs) {
    return lemma{{rs.begin(), rs.end()}};
}

} // namespace

using TestCdcl = cdcl_elimination_generator<chosen_goal_candidates, chosen_goal_candidates>;

struct CdclEliminationGeneratorUnitTest : public ::testing::Test {
protected:
    chosen_goal_candidates chosen;
    TestCdcl cdcl{chosen, chosen};

    void end_sim() {
        cdcl.cleanup();
        chosen.clear();
    }

    goal_lineage lin_0{nullptr, 0};
    goal_lineage lin_1{nullptr, 1};
    goal_lineage lin_2{nullptr, 2};
    goal_lineage lin_3{nullptr, 3};

    resolution_lineage lin_0_0{&lin_0, 0};
    resolution_lineage lin_0_1{&lin_0, 4};
    resolution_lineage lin_1_0{&lin_1, 1};
    resolution_lineage lin_2_0{&lin_2, 2};
    resolution_lineage lin_3_0{&lin_3, 3};

    goal_lineage lin_4{nullptr, 4};
    resolution_lineage lin_4_0{&lin_4, 5};
    goal_lineage lin_4_0_0{&lin_4_0, 0};
    goal_lineage lin_4_0_1{&lin_4_0, 1};
    resolution_lineage lin_4_0_0_0{&lin_4_0_0, 6};
    resolution_lineage lin_4_0_1_0{&lin_4_0_1, 7};
};

TEST_F(CdclEliminationGeneratorUnitTest, LearnUnitAvoidanceReturnsEliminationWithoutStoring) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0})), std::optional{&lin_0_0});
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnMultiMemberAvoidanceReturnsNull) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnDuplicateAvoidanceStoresTwice) {
    const lemma l = make_lemma({&lin_0_0, &lin_1_0});
    EXPECT_EQ(cdcl.learn(l), std::nullopt);
    EXPECT_EQ(cdcl.learn(l), std::nullopt);
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0, &lin_1_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnThreeIndependentPairAvoidances) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_2_0, &lin_3_0})), std::nullopt);
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_4_0_0_0, &lin_4_0_1_0})), std::nullopt);
}

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainWithNoLearnedAvoidancesYieldsNothing) {
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainAfterUnitLearnYieldsNothing) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0})), std::optional{&lin_0_0});
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainMember1YieldsMember2InBinaryAvoidance) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, SecondConstrainOnSameGoalYieldsNothing) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainMutuallyExclusiveResolutionErasesWithoutYield) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorUnitTest, TwoIndependentAvoidancesConstrainIndependently) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_2_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, SequentialConstrainOnDisjointIndependentAvoidances) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_2_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainOnGoalWithNoLearnedAvoidanceYieldsNothing) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_3_0)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainYieldsFromConsistentAvoidanceAndNotMutuallyExclusiveAvoidance) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_0_1, &lin_2_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), ElementsAre(&lin_2_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, DependentAndIndependentAvoidancesDoNotInterfere) {
    cdcl.learn(make_lemma({&lin_4_0_0_0, &lin_4_0_1_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_4_0_0_0)), ElementsAre(&lin_4_0_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, ThreeMemberAvoidanceSequentialConstrainEventuallyYieldsLast) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), ElementsAre(&lin_2_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnManyAvoidancesThenConstrainOneResolutionPerGoal) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_2_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_0_1, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_4_0_0_0, &lin_4_0_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_4_0_0_0)), ElementsAre(&lin_4_0_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), ElementsAre(&lin_2_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorUnitTest,
    FourAvoidancesSharingLin0ConstrainMemberLin0_0YieldsFromConsistentAvoidances) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_0_1, &lin_3_0}));
    EXPECT_THAT(
        collect_elims(cdcl.constrain(&lin_0_0)),
        UnorderedElementsAre(&lin_1_0, &lin_2_0, &lin_3_0));
}

TEST_F(CdclEliminationGeneratorUnitTest,
    FourAvoidancesSharingLin0ConstrainMemberLin0_1YieldsFromConsistentAvoidance) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_0_1, &lin_3_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorUnitTest,
    FourAvoidancesSharingLin0ConstrainExclusiveLin0_1ErasesOnlyLin0_0Avoidances) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorUnitTest,
    FourAvoidancesSharingLin0ConstrainExclusiveLin0_0ErasesOnlyLin0_1Avoidance) {
    cdcl.learn(make_lemma({&lin_0_1, &lin_3_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorUnitTest, SecondSiblingResolutionOnSameGoalYieldsNothingAfterUnwatch) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorUnitTest, SecondSiblingOnSameGoalNoOpWhenReducedAvoidanceRemains) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), ElementsAre(&lin_2_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, ReduceToDuplicateAvoidanceYieldsFromEachCopy) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_1_0, &lin_2_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), ElementsAre(&lin_2_0, &lin_2_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, FourMemberAvoidanceSequentialConstrainEventuallyYieldsLast) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, ExclusiveConstrainOnOneGoalLeavesOtherGoalAvoidanceIntact) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_2_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, CleanupRestoresSatisfiedAvoidanceForNextSim) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, CleanupRestoresThreeMemberAvoidanceForNextSim) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), ElementsAre(&lin_2_0));
    end_sim();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), ElementsAre(&lin_2_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, TerminalAvoidanceDoesNotRefireWithinSameSim) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), IsEmpty());
}
