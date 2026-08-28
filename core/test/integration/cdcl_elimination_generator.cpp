// cdcl_elimination_generator integration: persistent avoidance store with per-sim cleanup.
// Trail push/pop no longer affects CDCL state; cleanup() resets watcher layout at sim end.
// Tests parameterize over base and fgt (SIZE_MAX capacity) to prove both satisfy the same contract.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <vector>
#include "infrastructure/cdcl_elimination_generator.hpp"
#include "infrastructure/fgt_cdcl_elimination_generator.hpp"
#include "infrastructure/chosen_goal_candidates.hpp"
#include "infrastructure/elimination_backlog.hpp"
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

enum class cdcl_kind { base, fgt };

struct CdclIntegrationTest : public ::testing::TestWithParam<cdcl_kind> {
    elimination_backlog frames;
    chosen_goal_candidates chosen;
    std::optional<cdcl_elimination_generator<chosen_goal_candidates>> base_;
    std::optional<fgt_cdcl_elimination_generator<chosen_goal_candidates>> fgt_;

    void SetUp() override {
        if (GetParam() == cdcl_kind::base) base_.emplace(chosen);
        else                               fgt_.emplace(chosen, SIZE_MAX);
    }

    std::optional<const resolution_lineage*> learn(const lemma& l) {
        if (GetParam() == cdcl_kind::base) return base_->learn(l);
        return fgt_->learn(l);
    }

    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage* rl) {
        if (GetParam() == cdcl_kind::base) return base_->constrain(rl);
        return fgt_->constrain(rl);
    }

    void end_sim() {
        if (GetParam() == cdcl_kind::base) base_->cleanup();
        else                               fgt_->cleanup();
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
    resolution_lineage lin_4_0_0_1{&lin_4_0_0, 8};
    resolution_lineage lin_4_0_1_0{&lin_4_0_1, 7};
};

INSTANTIATE_TEST_SUITE_P(
    AllCdcl,
    CdclIntegrationTest,
    ::testing::Values(cdcl_kind::base, cdcl_kind::fgt),
    [](const ::testing::TestParamInfo<cdcl_kind>& info) {
        switch (info.param) {
            case cdcl_kind::base: return "base";
            case cdcl_kind::fgt:  return "fgt";
        }
        return "unknown";
    });

TEST_P(CdclIntegrationTest, AvoidanceSurvivesTrailPopAcrossEmptyFrame) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));

    frames.push_frame();
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_P(CdclIntegrationTest, LearnManyAvoidancesSurvivesEmptyPop) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    learn(make_lemma({&lin_2_0, &lin_3_0}));
    learn(make_lemma({&lin_0_1, &lin_2_0}));
    learn(make_lemma({&lin_4_0_0_0, &lin_4_0_1_0}));

    frames.push_frame();
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_4_0_0_0)), ElementsAre(&lin_4_0_1_0));
    EXPECT_THAT(collect_elims(constrain(&lin_0_1)), ElementsAre(&lin_2_0));
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_P(CdclIntegrationTest, LearnInFrameSurvivesPop) {
    frames.push_frame();
    EXPECT_EQ(learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_P(CdclIntegrationTest, LearnBeforePushSurvivesPopIncludingInnerLearn) {
    EXPECT_EQ(learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);

    frames.push_frame();
    EXPECT_EQ(learn(make_lemma({&lin_2_0, &lin_3_0})), std::nullopt);
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_P(CdclIntegrationTest, ConstrainInFramePersistsAcrossPop) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));

    frames.push_frame();
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_1_0)), IsEmpty());
}

TEST_P(CdclIntegrationTest, ExclusiveConstrainInFramePersistsUntilCleanup) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));

    frames.push_frame();
    EXPECT_THAT(collect_elims(constrain(&lin_0_1)), IsEmpty());
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_P(CdclIntegrationTest, LearnAndConstrainInSameFramePersistUntilCleanup) {
    frames.push_frame();
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_P(CdclIntegrationTest, MultipleLearnsInFrameAllSurvivePop) {
    frames.push_frame();
    EXPECT_EQ(learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);
    EXPECT_EQ(learn(make_lemma({&lin_2_0, &lin_3_0})), std::nullopt);
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_P(CdclIntegrationTest, MultipleConstrainsInFramePersistUntilCleanup) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    learn(make_lemma({&lin_2_0, &lin_3_0}));

    frames.push_frame();
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_P(CdclIntegrationTest, ThreeMemberAvoidancePartialConstrainRestoredByCleanup) {
    learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));

    frames.push_frame();
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_1_0)), ElementsAre(&lin_2_0));
    end_sim();
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    chosen.set(lin_0_0.parent, lin_0_0.idx);
    EXPECT_THAT(collect_elims(constrain(&lin_1_0)), ElementsAre(&lin_2_0));
}

TEST_P(CdclIntegrationTest, FourAvoidancesSharingGoalConstrainPersistsUntilCleanup) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));
    learn(make_lemma({&lin_0_0, &lin_2_0}));
    learn(make_lemma({&lin_0_0, &lin_3_0}));
    learn(make_lemma({&lin_0_1, &lin_3_0}));

    frames.push_frame();
    EXPECT_THAT(
        collect_elims(constrain(&lin_0_0)),
        UnorderedElementsAre(&lin_1_0, &lin_2_0, &lin_3_0));
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(
        collect_elims(constrain(&lin_0_0)),
        UnorderedElementsAre(&lin_1_0, &lin_2_0, &lin_3_0));
}

TEST_P(CdclIntegrationTest, TwoNestedFramesInnerLearnSurvivesInnerPop) {
    EXPECT_EQ(learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);

    frames.push_frame();
    frames.push_frame();
    EXPECT_EQ(learn(make_lemma({&lin_2_0, &lin_3_0})), std::nullopt);
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_P(CdclIntegrationTest, FourMemberAvoidancePartialConstrainRestoredByCleanup) {
    learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0, &lin_3_0}));

    frames.push_frame();
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_1_0)), IsEmpty());
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_1_0)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
    end_sim();
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    chosen.set(lin_0_0.parent, lin_0_0.idx);
    EXPECT_THAT(collect_elims(constrain(&lin_1_0)), IsEmpty());
    chosen.set(lin_1_0.parent, lin_1_0.idx);
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_P(CdclIntegrationTest, DependentTreeExclusiveConstrainRestoredByCleanup) {
    learn(make_lemma({&lin_4_0_0_0, &lin_4_0_1_0}));

    frames.push_frame();
    EXPECT_THAT(collect_elims(constrain(&lin_4_0_0_1)), IsEmpty());
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_4_0_0_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(constrain(&lin_4_0_0_0)), ElementsAre(&lin_4_0_1_0));
}

TEST_P(CdclIntegrationTest, LearnAfterConstrainInFramePersistsUntilCleanup) {
    learn(make_lemma({&lin_0_0, &lin_1_0}));

    frames.push_frame();
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    learn(make_lemma({&lin_2_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
    frames.pop_frame();

    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}
