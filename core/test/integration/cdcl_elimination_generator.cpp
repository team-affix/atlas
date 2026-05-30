// cdcl_elimination_generator + trail integration: backtrack undo of avoidance stores and
// watched-goal indexes. Asserts post-pop observable eliminations; learn/constrain logic is covered in unit tests.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "locator_fixture.hpp"
#include "infrastructure/cdcl_elimination_generator.hpp"
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

// Lineage naming matches unit tests: lin_i = ith root goal; lin_i_j = jth resolution of goal i.
struct CdclEliminationGeneratorIntegrationTest : public ::testing::Test {
protected:
    trail t;
    locator loc;
    cdcl_elimination_generator cdcl;

    CdclEliminationGeneratorIntegrationTest()
        : cdcl(bind_and_make<cdcl_elimination_generator, i_log_to_current_trail_frame>(loc, t)) {}

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

// ---------------------------------------------------------------------------
// empty frame
// ---------------------------------------------------------------------------

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

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnDuplicateInFrameThenPopPreservesOuterLearn) {
    const lemma l = make_lemma({&lin_0_0, &lin_1_0});
    EXPECT_EQ(cdcl.learn(l), std::nullopt);

    t.push();
    EXPECT_EQ(cdcl.learn(l), std::nullopt);
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

// ---------------------------------------------------------------------------
// single frame undo
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnInFrameUndoneByPop) {
    t.push();
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnBeforePushSurvivesPop) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);

    t.push();
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_2_0, &lin_3_0})), std::nullopt);
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorIntegrationTest, ConstrainInFrameUndoneByPop) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));

    t.push();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, ExclusiveConstrainInFrameUndoneByPop) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));

    t.push();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), IsEmpty());
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_1)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnAndConstrainInSameFrameBothUndoneByPop) {
    t.push();
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorIntegrationTest, MultipleLearnsInFrameAllUndoneByPop) {
    t.push();
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_2_0, &lin_3_0})), std::nullopt);
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorIntegrationTest, MultipleConstrainsInFrameAllUndoneByPop) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_2_0, &lin_3_0}));

    t.push();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, ThreeMemberAvoidancePartialConstrainInFrameUndoneByPop) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));

    t.push();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), ElementsAre(&lin_2_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, FourAvoidancesSharingGoalConstrainInFrameUndoneByPop) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_0_1, &lin_3_0}));

    t.push();
    EXPECT_THAT(
        collect_elims(cdcl.constrain(&lin_0_0)),
        UnorderedElementsAre(&lin_1_0, &lin_2_0, &lin_3_0));
    t.pop();

    EXPECT_THAT(
        collect_elims(cdcl.constrain(&lin_0_0)),
        UnorderedElementsAre(&lin_1_0, &lin_2_0, &lin_3_0));
}

// ---------------------------------------------------------------------------
// nested frames
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorIntegrationTest, TwoNestedFramesInnerLearnUndoneByInnerPop) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);

    t.push();
    t.push();
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_2_0, &lin_3_0})), std::nullopt);
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), IsEmpty());
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, TwoNestedFramesInnerConstrainUndoneByInnerPop) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));

    t.push();
    t.push();
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    t.pop();

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), ElementsAre(&lin_1_0));
}
