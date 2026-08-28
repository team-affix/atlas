// bt_cdcl_elimination_generator learns NAND lemmas into a hash-consed binary
// tree and yields unit eliminations from constrain. Tests use the public API only.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/bt_cdcl_elimination_generator.hpp"
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

}

struct BtCdclEliminationGeneratorUnitTest : public ::testing::Test {
protected:
    bt_cdcl_elimination_generator cdcl;

    void end_sim() {
        cdcl.cleanup();
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

    goal_lineage lin_5{nullptr, 5};
    resolution_lineage lin_5_0{&lin_5, 8};
    goal_lineage lin_6{nullptr, 6};
    resolution_lineage lin_6_0{&lin_6, 9};
    goal_lineage lin_7{nullptr, 7};
    resolution_lineage lin_7_0{&lin_7, 10};
    goal_lineage lin_8{nullptr, 8};
    resolution_lineage lin_8_0{&lin_8, 11};
};

TEST_F(BtCdclEliminationGeneratorUnitTest, LearnUnitAvoidanceReturnsEliminationWithoutStoring) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0})), std::optional{&lin_0_0});
}

TEST_F(BtCdclEliminationGeneratorUnitTest, LearnMultiMemberAvoidanceReturnsNull) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);
}

TEST_F(BtCdclEliminationGeneratorUnitTest, LearnDuplicateAvoidanceIsIdempotent) {
    const lemma l = make_lemma({&lin_0_0, &lin_1_0});
    EXPECT_EQ(cdcl.learn(l), std::nullopt);
    EXPECT_EQ(cdcl.learn(l), std::nullopt);
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, ConstrainWithNoLearnedAvoidancesYieldsNothing) {
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
}

TEST_F(BtCdclEliminationGeneratorUnitTest, ConstrainAfterUnitLearnYieldsNothing) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0})), std::optional{&lin_0_0});
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
}

TEST_F(BtCdclEliminationGeneratorUnitTest, ConstrainMember1YieldsMember2InBinaryAvoidance) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, SecondConstrainOnSameGoalYieldsNothing) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
}

TEST_F(BtCdclEliminationGeneratorUnitTest, ConstrainMutuallyExclusiveResolutionIsNoOp) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), IsEmpty());
}

TEST_F(BtCdclEliminationGeneratorUnitTest, TwoIndependentAvoidancesConstrainIndependently) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_2_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, ConstrainOnGoalWithNoLearnedAvoidanceYieldsNothing) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_3_0)), IsEmpty());
}

TEST_F(BtCdclEliminationGeneratorUnitTest, ConstrainYieldsFromConsistentAvoidanceAndNotMutuallyExclusiveAvoidance) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_0_1, &lin_2_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), ElementsAre(&lin_2_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, DependentAndIndependentAvoidancesDoNotInterfere) {
    cdcl.learn(make_lemma({&lin_4_0_0_0, &lin_4_0_1_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_4_0_0_0)), ElementsAre(&lin_4_0_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, ThreeMemberAvoidanceSequentialConstrainEventuallyYieldsLast) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), ElementsAre(&lin_2_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), IsEmpty());
}

TEST_F(BtCdclEliminationGeneratorUnitTest, FourAvoidancesSharingLin0ConstrainMemberLin0_0YieldsFromConsistentAvoidances) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_0_1, &lin_3_0}));
    EXPECT_THAT(
        collect_elims(cdcl.constrain(&lin_0_0)),
        UnorderedElementsAre(&lin_1_0, &lin_2_0, &lin_3_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, FourAvoidancesSharingLin0ConstrainMemberLin0_1YieldsFromConsistentAvoidance) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_0_1, &lin_3_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), ElementsAre(&lin_3_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, ExclusiveConstrainOnOneGoalLeavesOtherGoalAvoidanceIntact) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_2_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, FourMemberAvoidanceSequentialConstrainEventuallyYieldsLast) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, CleanupRestoresSatisfiedAvoidanceForNextSim) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, CleanupRestoresThreeMemberAvoidanceForNextSim) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), ElementsAre(&lin_2_0));
    end_sim();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), ElementsAre(&lin_2_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, TerminalAvoidanceDoesNotRefireWithinSameSim) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), IsEmpty());
}

TEST_F(BtCdclEliminationGeneratorUnitTest, SharedPrefixFourSetsYieldOnlyTheTouchedTail) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_5_0, &lin_6_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_5_0)), ElementsAre(&lin_6_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, SizeFiveLeftoverAtTopFourthConstrainYieldsLeftover) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0, &lin_3_0, &lin_5_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_3_0)), ElementsAre(&lin_5_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, SharedPairDoesNotTouchIndependentNand) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_5_0, &lin_6_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_5_0)), ElementsAre(&lin_6_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, CleanupThenReplaySharedFourSetPath) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_5_0, &lin_6_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
    end_sim();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, ManyFourSetsSharingPrefixConstrainFirstMemberYieldsNothing) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_5_0, &lin_6_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_7_0, &lin_8_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
}

TEST_F(BtCdclEliminationGeneratorUnitTest, ReduceToDuplicateAvoidanceYieldsFromEachCopy) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_1_0, &lin_2_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), ElementsAre(&lin_2_0, &lin_2_0));
}

TEST_F(BtCdclEliminationGeneratorUnitTest, AbandonedConstrainDoesNotRefireUntilCleanup) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));

    auto sm = cdcl.constrain(&lin_0_0);
    auto first = sm.next();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(*first, &lin_1_0);

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}
