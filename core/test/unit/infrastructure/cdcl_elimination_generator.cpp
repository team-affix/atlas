// cdcl_elimination_generator learns pairwise avoidance lemmas and yields eliminations
// during constrain. Tests parameterize over base, fgt (SIZE_MAX capacity), and fgt_bt
// to prove all three types satisfy the same contract.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <climits>
#include <optional>
#include <vector>
#include "infrastructure/cdcl_elimination_generator.hpp"
#include "infrastructure/fgt_cdcl_elimination_generator.hpp"
#include "infrastructure/fgt_bt_cdcl_elimination_generator.hpp"
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

enum class cdcl_kind { base, fgt, fgt_bt };

struct CdclUnitTest : public ::testing::TestWithParam<cdcl_kind> {
    chosen_goal_candidates chosen;
    std::optional<cdcl_elimination_generator<chosen_goal_candidates>>     base_;
    std::optional<fgt_cdcl_elimination_generator<chosen_goal_candidates>> fgt_;
    std::optional<fgt_bt_cdcl_elimination_generator>                      fgt_bt_;

    void SetUp() override {
        switch (GetParam()) {
            case cdcl_kind::base:   base_.emplace(chosen);          break;
            case cdcl_kind::fgt:    fgt_.emplace(chosen, SIZE_MAX); break;
            case cdcl_kind::fgt_bt: fgt_bt_.emplace(SIZE_MAX);      break;
        }
    }

    std::optional<const resolution_lineage*> learn(const lemma& l) {
        switch (GetParam()) {
            case cdcl_kind::base:   return base_->learn(l);
            case cdcl_kind::fgt:    return fgt_->learn(l);
            case cdcl_kind::fgt_bt: return fgt_bt_->learn(l);
        }
        return std::nullopt;
    }

    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage* rl) {
        switch (GetParam()) {
            case cdcl_kind::base:   return base_->constrain(rl);
            case cdcl_kind::fgt:    return fgt_->constrain(rl);
            case cdcl_kind::fgt_bt: return fgt_bt_->constrain(rl);
        }
        return base_->constrain(rl);
    }

    void end_sim() {
        switch (GetParam()) {
            case cdcl_kind::base:   base_->cleanup();   break;
            case cdcl_kind::fgt:    fgt_->cleanup();    break;
            case cdcl_kind::fgt_bt: fgt_bt_->cleanup(); break;
        }
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

INSTANTIATE_TEST_SUITE_P(
    AllCdcl,
    CdclUnitTest,
    ::testing::Values(cdcl_kind::base, cdcl_kind::fgt, cdcl_kind::fgt_bt),
    [](const ::testing::TestParamInfo<cdcl_kind>& info) {
        switch (info.param) {
            case cdcl_kind::base:   return "base";
            case cdcl_kind::fgt:    return "fgt";
            case cdcl_kind::fgt_bt: return "fgt_bt";
        }
        return "unknown";
    });

TEST_P(CdclUnitTest, LearnUnitAvoidanceReturnsEliminationWithoutStoring) {
    EXPECT_EQ(learn(make_lemma({&lin_0_0})), std::optional{&lin_0_0});
}

TEST_P(CdclUnitTest, LearnMultiMemberAvoidanceReturnsNull) {
    EXPECT_EQ(learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);
}

TEST_P(CdclUnitTest, LearnDuplicateAvoidanceStoresTwice) {
    // fgt_bt interns by tree structure; a second learn of the same lemma is
    // idempotent and does not add a second copy to fire_order_.
    if (GetParam() == cdcl_kind::fgt_bt) GTEST_SKIP();
    const lemma l = make_lemma({&lin_0_0, &lin_1_0});
    EXPECT_EQ(learn(l), std::nullopt);
    EXPECT_EQ(learn(l), std::nullopt);
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0, &lin_1_0));
}

TEST_P(CdclUnitTest, LearnThreeIndependentPairAvoidances) {
    EXPECT_EQ(learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);
    EXPECT_EQ(learn(make_lemma({&lin_2_0, &lin_3_0})), std::nullopt);
    EXPECT_EQ(learn(make_lemma({&lin_4_0_0_0, &lin_4_0_1_0})), std::nullopt);
}

TEST_P(CdclUnitTest, ConstrainWithNoLearnedAvoidancesYieldsNothing) {
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
}

TEST_P(CdclUnitTest, ConstrainAfterUnitLearnYieldsNothing) {
    EXPECT_EQ(learn(make_lemma({&lin_0_0})), std::optional{&lin_0_0});
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
}

TEST_P(CdclUnitTest, ConstrainMember1YieldsMember2InBinaryAvoidance) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_P(CdclUnitTest, SecondConstrainOnSameGoalYieldsNothing) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
}

TEST_P(CdclUnitTest, ConstrainMutuallyExclusiveResolutionErasesWithoutYield) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_1)), IsEmpty());
}

TEST_P(CdclUnitTest, TwoIndependentAvoidancesConstrainIndependently) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    learn(make_lemma({&lin_2_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_P(CdclUnitTest, SequentialConstrainOnDisjointIndependentAvoidances) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    learn(make_lemma({&lin_2_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_P(CdclUnitTest, ConstrainOnGoalWithNoLearnedAvoidanceYieldsNothing) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_3_0)), IsEmpty());
}

TEST_P(CdclUnitTest, ConstrainYieldsFromConsistentAvoidanceAndNotMutuallyExclusiveAvoidance) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    learn(make_lemma({&lin_0_1, &lin_2_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_1)), ElementsAre(&lin_2_0));
}

TEST_P(CdclUnitTest, DependentAndIndependentAvoidancesDoNotInterfere) {
    learn(make_lemma({&lin_4_0_0_0, &lin_4_0_1_0}));
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_4_0_0_0)), ElementsAre(&lin_4_0_1_0));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_P(CdclUnitTest, ThreeMemberAvoidanceSequentialConstrainEventuallyYieldsLast) {
    learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_1_0)), ElementsAre(&lin_2_0));
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), IsEmpty());
}

TEST_P(CdclUnitTest, LearnManyAvoidancesThenConstrainOneResolutionPerGoal) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    learn(make_lemma({&lin_2_0, &lin_3_0}));
    learn(make_lemma({&lin_0_1, &lin_2_0}));
    learn(make_lemma({&lin_4_0_0_0, &lin_4_0_1_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_4_0_0_0)), ElementsAre(&lin_4_0_1_0));
    EXPECT_THAT(collect_elims(constrain(&lin_0_1)), ElementsAre(&lin_2_0));
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_P(CdclUnitTest,
    FourAvoidancesSharingLin0ConstrainMemberLin0_0YieldsFromConsistentAvoidances) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    learn(make_lemma({&lin_0_0, &lin_2_0}));
    learn(make_lemma({&lin_0_0, &lin_3_0}));
    learn(make_lemma({&lin_0_1, &lin_3_0}));
    EXPECT_THAT(
        collect_elims(constrain(&lin_0_0)),
        UnorderedElementsAre(&lin_1_0, &lin_2_0, &lin_3_0));
}

TEST_P(CdclUnitTest,
    FourAvoidancesSharingLin0ConstrainMemberLin0_1YieldsFromConsistentAvoidance) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    learn(make_lemma({&lin_0_0, &lin_2_0}));
    learn(make_lemma({&lin_0_0, &lin_3_0}));
    learn(make_lemma({&lin_0_1, &lin_3_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_1)), ElementsAre(&lin_3_0));
}

TEST_P(CdclUnitTest,
    FourAvoidancesSharingLin0ConstrainExclusiveLin0_1ErasesOnlyLin0_0Avoidances) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    learn(make_lemma({&lin_0_0, &lin_2_0}));
    learn(make_lemma({&lin_0_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_1)), IsEmpty());
}

TEST_P(CdclUnitTest,
    FourAvoidancesSharingLin0ConstrainExclusiveLin0_0ErasesOnlyLin0_1Avoidance) {
    learn(make_lemma({&lin_0_1, &lin_3_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
}

TEST_P(CdclUnitTest, SecondSiblingResolutionOnSameGoalYieldsNothingAfterUnwatch) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(constrain(&lin_0_1)), IsEmpty());
}

TEST_P(CdclUnitTest, SecondSiblingOnSameGoalNoOpWhenReducedAvoidanceRemains) {
    learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_0_1)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_1_0)), ElementsAre(&lin_2_0));
}

TEST_P(CdclUnitTest, ReduceToDuplicateAvoidanceYieldsFromEachCopy) {
    learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));
    learn(make_lemma({&lin_1_0, &lin_2_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_1_0)), ElementsAre(&lin_2_0, &lin_2_0));
}

TEST_P(CdclUnitTest, FourMemberAvoidanceSequentialConstrainEventuallyYieldsLast) {
    learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_1_0)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_P(CdclUnitTest, ExclusiveConstrainOnOneGoalLeavesOtherGoalAvoidanceIntact) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    learn(make_lemma({&lin_2_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_1)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_P(CdclUnitTest, CleanupRestoresSatisfiedAvoidanceForNextSim) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_1)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_P(CdclUnitTest, CleanupRestoresThreeMemberAvoidanceForNextSim) {
    learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_1_0)), ElementsAre(&lin_2_0));
    end_sim();
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    chosen.set(lin_0_0.parent, lin_0_0.idx);
    EXPECT_THAT(collect_elims(constrain(&lin_1_0)), ElementsAre(&lin_2_0));
}

TEST_P(CdclUnitTest, TerminalAvoidanceDoesNotRefireWithinSameSim) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(constrain(&lin_1_0)), IsEmpty());
}
