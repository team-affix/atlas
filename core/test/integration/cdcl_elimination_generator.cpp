#include <gtest/gtest.h>
#include <algorithm>
#include <vector>
#include "../../../core/hpp/infrastructure/cdcl_elimination_generator.hpp"
#include "../../../core/hpp/infrastructure/lineage_pool.hpp"
#include "../../../core/hpp/utility/trail.hpp"
#include "../../../core/hpp/utility/state_machine.hpp"

namespace {

std::vector<const resolution_lineage*> collect_elims(
    state_machine<const resolution_lineage*>& sm) {
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
    lineage_pool lp;
    cdcl_elimination_generator cdcl{t};

    // Mutually consistent avoidances: each resolution has a distinct parent goal.
    expr goal_expr0{expr::var{0}};
    expr goal_expr1{expr::var{1}};
    expr head0{expr::var{10}};
    expr head1{expr::var{11}};
    rule rule0{&head0, {}};
    rule rule1{&head1, {}};
    rule rule2{&head1, {}};

    const goal_lineage* gl0 = nullptr;
    const goal_lineage* gl1 = nullptr;
    const resolution_lineage* rl0 = nullptr;
    const resolution_lineage* rl1 = nullptr;

    void SetUp() override {
        gl0 = lp.goal(nullptr, &goal_expr0);
        gl1 = lp.goal(nullptr, &goal_expr1);
        rl0 = lp.resolution(gl0, &rule0);
        rl1 = lp.resolution(gl1, &rule1);
    }
};

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnStoresAvoidanceForConstrain) {
    lemma l{{rl0, rl1}};
    EXPECT_EQ(cdcl.learn(l), nullptr);

    auto sm = cdcl.constrain(rl0);
    auto elims = collect_elims(sm);

    EXPECT_EQ(elims, (std::vector<const resolution_lineage*>{rl1}));
}

TEST_F(CdclEliminationGeneratorIntegrationTest, LearnDuplicateAvoidanceIsIdempotent) {
    lemma l{{rl0, rl1}};
    EXPECT_EQ(cdcl.learn(l), nullptr);
    EXPECT_EQ(cdcl.learn(l), nullptr);
}

TEST_F(CdclEliminationGeneratorIntegrationTest, ConstrainMutuallyExclusiveAvoidanceErasesWithoutYield) {
    const resolution_lineage* rl2 = lp.resolution(gl0, &rule2);

    lemma l{{rl0, rl1}};
    cdcl.learn(l);

    auto sm = cdcl.constrain(rl2);
    auto elims = collect_elims(sm);

    EXPECT_TRUE(elims.empty());
}

TEST_F(CdclEliminationGeneratorIntegrationTest, AvoidanceSurvivesTrailPopAcrossFrames) {
    lemma l{{rl0, rl1}};
    cdcl.learn(l);

    t.push();
    t.pop();

    auto sm = cdcl.constrain(rl0);
    auto elims = collect_elims(sm);

    EXPECT_EQ(elims, (std::vector<const resolution_lineage*>{rl1}));
}
