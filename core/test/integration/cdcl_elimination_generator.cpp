#include <gtest/gtest.h>
#include <vector>
#include "../../../core/hpp/infrastructure/cdcl_elimination_generator.hpp"
#include "../../../core/hpp/utility/trail.hpp"
#include "../../../core/hpp/utility/state_machine.hpp"

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
    cdcl_elimination_generator cdcl{t};

    expr goal_expr0{expr::var{0}};
    expr goal_expr1{expr::var{1}};
    expr goal_expr2{expr::var{2}};
    expr goal_expr3{expr::var{3}};
    expr head0{expr::var{10}};
    expr head1{expr::var{11}};
    expr head2{expr::var{12}};
    expr head3{expr::var{13}};
    expr head0_alt{expr::var{14}};

    rule rule0{&head0, {}};
    rule rule1{&head1, {}};
    rule rule2{&head2, {}};
    rule rule3{&head3, {}};
    rule rule0_alt{&head0_alt, {}};

    goal_lineage gl0{nullptr, &goal_expr0};
    goal_lineage gl1{nullptr, &goal_expr1};
    goal_lineage gl2{nullptr, &goal_expr2};
    goal_lineage gl3{nullptr, &goal_expr3};

    resolution_lineage rl0{&gl0, &rule0};
    resolution_lineage rl1{&gl1, &rule1};
    resolution_lineage rl2{&gl2, &rule2};
    resolution_lineage rl3{&gl3, &rule3};
    resolution_lineage rl0_alt{&gl0, &rule0_alt};
};

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnStoresAvoidanceForConstrain) {
    lemma l{{&rl0, &rl1}};
    EXPECT_EQ(cdcl.learn(l), nullptr);

    auto elims = collect_elims(cdcl.constrain(&rl0));

    EXPECT_EQ(elims, (std::vector<const resolution_lineage*>{&rl1}));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnDuplicateAvoidanceIsIdempotent) {
    lemma l{{&rl0, &rl1}};
    EXPECT_EQ(cdcl.learn(l), nullptr);
    EXPECT_EQ(cdcl.learn(l), nullptr);
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

    EXPECT_EQ(cdcl.learn(outside), nullptr);

    t.push();
    EXPECT_EQ(cdcl.learn(inside), nullptr);
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

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnAndConstrainInFrameFullyUndoneByPop) {
    const lemma baseline{{&rl0, &rl1}};
    const lemma in_frame{{&rl2, &rl3}};

    cdcl.learn(baseline);

    t.push();
    cdcl.learn(in_frame);
    collect_elims(cdcl.constrain(&rl2));
    t.pop();

    EXPECT_EQ(cdcl.learn(in_frame), nullptr);
    auto elims = collect_elims(cdcl.constrain(&rl0));
    EXPECT_EQ(elims, (std::vector<const resolution_lineage*>{&rl1}));
}
