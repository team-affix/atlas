#include <gtest/gtest.h>
#include "locator_fixture.hpp"
#include <vector>
#include "infrastructure/cdcl_elimination_generator.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/state_machine.hpp"

namespace {

std::vector<const resolution_lineage*> collect_elims(
    state_machine<const resolution_lineage*> sm) {
    std::vector<const resolution_lineage*> out;
    while (!sm.done()) {
        auto v = sm.resume();
        if (v.has_value() && v.value() != nullptr)
            out.push_back(v.value());
    }
    return out;
}

} // namespace

struct CdclEliminationGeneratorIntegrationTest : public ::testing::Test {
protected:
    trail t;
    locator loc;
    cdcl_elimination_generator cdcl;

    CdclEliminationGeneratorIntegrationTest()
        : cdcl(bind_and_make<cdcl_elimination_generator, i_log_to_current_trail_frame>(loc, t)) {}

    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
    goal_lineage gl2{nullptr, 2};
    goal_lineage gl3{nullptr, 3};

    resolution_lineage rl0{&gl0, 0};
    resolution_lineage rl1{&gl1, 1};
    resolution_lineage rl2{&gl2, 2};
    resolution_lineage rl3{&gl3, 3};
    resolution_lineage rl0_alt{&gl0, 4};
};

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnStoresAvoidanceForConstrain) {
    lemma l{{&rl0, &rl1}};
    EXPECT_EQ(cdcl.learn(l), std::nullopt);

    auto elims = collect_elims(cdcl.constrain(&rl0));

    EXPECT_EQ(elims, (std::vector<const resolution_lineage*>{&rl1}));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnDuplicateAvoidanceIsIdempotent) {
    lemma l{{&rl0, &rl1}};
    EXPECT_EQ(cdcl.learn(l), std::nullopt);
    EXPECT_EQ(cdcl.learn(l), std::nullopt);
}

TEST_F(CdclEliminationGeneratorIntegrationTest, ConstrainMutuallyExclusiveAvoidanceErasesWithoutYield) {
    lemma l{{&rl0, &rl1}};
    cdcl.learn(l);

    auto elims = collect_elims(cdcl.constrain(&rl0_alt));

    EXPECT_TRUE(elims.empty());
}

TEST_F(CdclEliminationGeneratorIntegrationTest, AvoidanceSurvivesTrailPopAcrossEmptyFrame) {
    lemma l{{&rl0, &rl1}};
    cdcl.learn(l);

    t.push();
    t.pop();

    auto elims = collect_elims(cdcl.constrain(&rl0));

    EXPECT_EQ(elims, (std::vector<const resolution_lineage*>{&rl1}));
}

// ---------------------------------------------------------------------------
// trail — real cdcl_elimination_generator + real trail
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnBeforePushSurvivesPop) {
    const lemma outside{{&rl0, &rl1}};
    const lemma inside{{&rl2, &rl3}};

    EXPECT_EQ(cdcl.learn(outside), std::nullopt);

    t.push();
    EXPECT_EQ(cdcl.learn(inside), std::nullopt);
    t.pop();

    auto elims1 = collect_elims(cdcl.constrain(&rl0));
    auto elims2 = collect_elims(cdcl.constrain(&rl2));
    
    EXPECT_EQ(elims1, (std::vector<const resolution_lineage*>{&rl1}));
    EXPECT_EQ(elims2, (std::vector<const resolution_lineage*>{}));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, ConstrainInFrameUndoneByPop) {
    const lemma l{{&rl0, &rl1}};
    cdcl.learn(l);

    t.push();
    EXPECT_EQ(collect_elims(cdcl.constrain(&rl0)),
        (std::vector<const resolution_lineage*>{&rl1}));
    t.pop();

    auto elims = collect_elims(cdcl.constrain(&rl0));
    EXPECT_EQ(elims, (std::vector<const resolution_lineage*>{&rl1}));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, ExclusiveConstrainInFrameUndoneByPop) {
    const lemma l{{&rl0, &rl1}};
    cdcl.learn(l);

    t.push();
    EXPECT_TRUE(collect_elims(cdcl.constrain(&rl0_alt)).empty());
    t.pop();

    auto elims = collect_elims(cdcl.constrain(&rl0));
    EXPECT_EQ(elims, (std::vector<const resolution_lineage*>{&rl1}));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnInFrameUndoneByPop) {
    const lemma l{{&rl0, &rl1}};

    t.push();
    EXPECT_EQ(cdcl.learn(l), std::nullopt);
    t.pop();

    auto elims = collect_elims(cdcl.constrain(&rl0));
    EXPECT_TRUE(elims.empty());
}
