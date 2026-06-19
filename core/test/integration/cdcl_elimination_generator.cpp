// cdcl_elimination_generator integration: persistent avoidance store with per-sim cleanup.
// Trail push/pop no longer affects CDCL state; cleanup() resets watcher layout at sim end.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "locator_fixture.hpp"
#include "infrastructure/cdcl_elimination_generator.hpp"
#include "infrastructure/chosen_goal_candidates.hpp"
#include "infrastructure/trail.hpp"
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

struct CdclEliminationGeneratorIntegrationTest : public ::testing::Test {
protected:
    trail t;
    locator loc;
    chosen_goal_candidates chosen;
    cdcl_elimination_generator cdcl;

    CdclEliminationGeneratorIntegrationTest()
        : chosen(),
          cdcl([&]() {
              loc.bind_as<i_try_get_chosen_goal_candidate, i_set_chosen_goal_candidate,
                  i_clear_chosen_goal_candidates>(chosen);
              return cdcl_elimination_generator{loc};
          }()) {
        loc.bind_as<i_learn_avoidance, i_clean_up_cdcl>(cdcl);
    }

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
    resolution_lineage lin_4_0_0_1{&lin_4_0_0, 8};
    resolution_lineage lin_4_0_1_0{&lin_4_0_1, 7};
};

TEST_F(CdclEliminationGeneratorIntegrationTest, AvoidanceSurvivesTrailPopAcrossEmptyFrame) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));

    t.push();
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnManyAvoidancesSurvivesEmptyPop) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_2_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_0_1, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_4_0_0_0, &lin_4_0_1_0}));

    t.push();
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_4_0_0_0)), ElementsAre(&lin_4_0_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), ElementsAre(&lin_2_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnInFrameSurvivesPop) {
    t.push();
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnBeforePushSurvivesPopIncludingInnerLearn) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);

    t.push();
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_2_0, &lin_3_0})), std::nullopt);
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, ConstrainInFramePersistsAcrossPop) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));

    t.push();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorIntegrationTest, ExclusiveConstrainInFramePersistsUntilCleanup) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));

    t.push();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), IsEmpty());
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnAndConstrainInSameFramePersistUntilCleanup) {
    t.push();
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, MultipleLearnsInFrameAllSurvivePop) {
    t.push();
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_2_0, &lin_3_0})), std::nullopt);
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, MultipleConstrainsInFramePersistUntilCleanup) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_2_0, &lin_3_0}));

    t.push();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, ThreeMemberAvoidancePartialConstrainRestoredByCleanup) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));

    t.push();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), ElementsAre(&lin_2_0));
    end_sim();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), ElementsAre(&lin_2_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, FourAvoidancesSharingGoalConstrainPersistsUntilCleanup) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_0_1, &lin_3_0}));

    t.push();
    EXPECT_THAT(
        collect_elims(cdcl.constrain(&lin_0_0)),
        UnorderedElementsAre(&lin_1_0, &lin_2_0, &lin_3_0));
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(
        collect_elims(cdcl.constrain(&lin_0_0)),
        UnorderedElementsAre(&lin_1_0, &lin_2_0, &lin_3_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, TwoNestedFramesInnerLearnSurvivesInnerPop) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);

    t.push();
    t.push();
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_2_0, &lin_3_0})), std::nullopt);
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, FourMemberAvoidancePartialConstrainRestoredByCleanup) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0, &lin_3_0}));

    t.push();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), IsEmpty());
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
    end_sim();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, DependentTreeExclusiveConstrainRestoredByCleanup) {
    cdcl.learn(make_lemma({&lin_4_0_0_0, &lin_4_0_1_0}));

    t.push();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_4_0_0_1)), IsEmpty());
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_4_0_0_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_4_0_0_0)), ElementsAre(&lin_4_0_1_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnAfterConstrainInFramePersistsUntilCleanup) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));

    t.push();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    cdcl.learn(make_lemma({&lin_2_0, &lin_3_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), IsEmpty());
    end_sim();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}
