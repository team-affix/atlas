#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "../../../core/hpp/infrastructure/cdcl_elimination_generator.hpp"
#include "../../../core/hpp/utility/i_trail.hpp"
#include "../../../core/hpp/utility/state_machine.hpp"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::StrictMock;
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

lemma make_lemma(std::initializer_list<const resolution_lineage*> rs) {
    return lemma{{rs.begin(), rs.end()}};
}

} // namespace

struct MockTrail : public i_trail {
    MOCK_METHOD(void, push, (), (override));
    MOCK_METHOD(void, pop, (), (override));
    MOCK_METHOD(void, log, ((std::unique_ptr<i_backtrackable>)), (override));
};

// Mutually consistent avoidances only: at most one resolution per parent goal in each set.
struct CdclEliminationGeneratorUnitTest : public ::testing::Test {
protected:
    NiceMock<MockTrail> trail;
    cdcl_elimination_generator cdcl{trail};

    // Independent goals (sibling roots).
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

    // Dependent goals: gl_child derived via res_mid from gl_root.
    expr goal_expr_root{expr::var{20}};
    expr goal_expr_child{expr::var{21}};
    expr head_root{expr::var{30}};
    expr head_child{expr::var{31}};

    rule rule_root{&head_root, {}};
    rule rule_child{&head_child, {}};

    goal_lineage gl_root{nullptr, &goal_expr_root};
    resolution_lineage res_mid{&gl_root, &rule_root};
    goal_lineage gl_child{&res_mid, &goal_expr_child};
    resolution_lineage rl_root{&gl_root, &rule_root};
    resolution_lineage rl_child{&gl_child, &rule_child};
};

// ---------------------------------------------------------------------------
// learn
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, LearnUnitAvoidanceReturnsEliminationWithoutStoring) {
    EXPECT_EQ(cdcl.learn(make_lemma({&rl0})), &rl0);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnMultiMemberAvoidanceReturnsNull) {
    EXPECT_EQ(cdcl.learn(make_lemma({&rl0, &rl1})), nullptr);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnUnitAvoidanceDoesNotLogToTrail) {
    StrictMock<MockTrail> strict_trail;
    cdcl_elimination_generator strict_cdcl{strict_trail};

    EXPECT_CALL(strict_trail, log(_)).Times(0);
    EXPECT_EQ(strict_cdcl.learn(make_lemma({&rl0})), &rl0);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnMultiMemberAvoidanceLogsToTrail) {
    StrictMock<MockTrail> strict_trail;
    cdcl_elimination_generator strict_cdcl{strict_trail};

    EXPECT_CALL(strict_trail, log(_)).Times(AtLeast(1));
    EXPECT_EQ(strict_cdcl.learn(make_lemma({&rl0, &rl1})), nullptr);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnDuplicateAvoidanceIsIdempotent) {
    const lemma l = make_lemma({&rl0, &rl1});
    EXPECT_EQ(cdcl.learn(l), nullptr);
    EXPECT_EQ(cdcl.learn(l), nullptr);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnDuplicateAvoidanceDoesNotLogToTrail) {
    StrictMock<MockTrail> strict_trail;
    cdcl_elimination_generator strict_cdcl{strict_trail};

    const lemma l = make_lemma({&rl0, &rl1});

    EXPECT_CALL(strict_trail, log(_)).Times(AtLeast(1));
    EXPECT_EQ(strict_cdcl.learn(l), nullptr);
    testing::Mock::VerifyAndClearExpectations(&strict_trail);

    EXPECT_CALL(strict_trail, log(_)).Times(0);
    EXPECT_EQ(strict_cdcl.learn(l), nullptr);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnThreeIndependentPairAvoidances) {
    EXPECT_EQ(cdcl.learn(make_lemma({&rl0, &rl1})), nullptr);
    EXPECT_EQ(cdcl.learn(make_lemma({&rl2, &rl3})), nullptr);
    EXPECT_EQ(cdcl.learn(make_lemma({&rl_root, &rl_child})), nullptr);
}

// ---------------------------------------------------------------------------
// constrain — no prior learn
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainWithNoLearnedAvoidancesYieldsNothing) {
    auto elims = collect_elims(cdcl.constrain(&rl0));
    EXPECT_THAT(elims, IsEmpty());
}

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainAfterUnitLearnYieldsNothing) {
    EXPECT_EQ(cdcl.learn(make_lemma({&rl0})), &rl0);
    auto elims = collect_elims(cdcl.constrain(&rl0));
    EXPECT_THAT(elims, IsEmpty());
}

// ---------------------------------------------------------------------------
// constrain — single stored avoidance
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainMember1YieldsMember2InBinaryAvoidance) {
    cdcl.learn(make_lemma({&rl0, &rl1}));

    auto elims = collect_elims(cdcl.constrain(&rl0));

    EXPECT_THAT(elims, ElementsAre(&rl1));
}

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainMutuallyExclusiveResolutionErasesWithoutYield) {
    cdcl.learn(make_lemma({&rl0, &rl1}));

    auto elims = collect_elims(cdcl.constrain(&rl0_alt));

    EXPECT_THAT(elims, IsEmpty());
}

// ---------------------------------------------------------------------------
// constrain — multiple learned avoidances (independent)
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, TwoIndependentAvoidancesConstrainIndependently) {
    cdcl.learn(make_lemma({&rl0, &rl1}));
    cdcl.learn(make_lemma({&rl2, &rl3}));

    auto from_gl0 = collect_elims(cdcl.constrain(&rl0));
    auto from_gl2 = collect_elims(cdcl.constrain(&rl2));

    EXPECT_THAT(from_gl0, ElementsAre(&rl1));
    EXPECT_THAT(from_gl2, ElementsAre(&rl3));
}

TEST_F(CdclEliminationGeneratorUnitTest, SequentialConstrainOnDisjointIndependentAvoidances) {
    cdcl.learn(make_lemma({&rl0, &rl1}));
    cdcl.learn(make_lemma({&rl2, &rl3}));

    auto first = collect_elims(cdcl.constrain(&rl0));
    auto second = collect_elims(cdcl.constrain(&rl2));

    EXPECT_THAT(first, ElementsAre(&rl1));
    EXPECT_THAT(second, ElementsAre(&rl3));
}

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainOnGoalWithNoLearnedAvoidanceYieldsNothing) {
    cdcl.learn(make_lemma({&rl0, &rl1}));

    auto elims = collect_elims(cdcl.constrain(&rl3));

    EXPECT_THAT(elims, IsEmpty());
}

// ---------------------------------------------------------------------------
// constrain — two avoidances watching gl0 (one constrain per goal: pick one resolution)
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainYieldsFromConsistentAvoidanceAndNotMutuallyExclusiveAvoidance) {
    cdcl.learn(make_lemma({&rl0, &rl1}));
    cdcl.learn(make_lemma({&rl0_alt, &rl2}));

    auto elims = collect_elims(cdcl.constrain(&rl0_alt));

    EXPECT_THAT(elims, ElementsAre(&rl2));
}

// ---------------------------------------------------------------------------
// constrain — dependent lineage (distinct parents within each avoidance)
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, DependentAndIndependentAvoidancesDoNotInterfere) {
    cdcl.learn(make_lemma({&rl_root, &rl_child}));
    cdcl.learn(make_lemma({&rl0, &rl1}));

    auto from_dependent = collect_elims(cdcl.constrain(&rl_root));
    auto from_independent = collect_elims(cdcl.constrain(&rl0));

    EXPECT_THAT(from_dependent, ElementsAre(&rl_child));
    EXPECT_THAT(from_independent, ElementsAre(&rl1));
}

// ---------------------------------------------------------------------------
// constrain — sequential reduction of a three-member avoidance
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, ThreeMemberAvoidanceSequentialConstrainEventuallyYieldsLast) {
    cdcl.learn(make_lemma({&rl0, &rl1, &rl2}));

    EXPECT_THAT(collect_elims(cdcl.constrain(&rl0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&rl1)), ElementsAre(&rl2));
    EXPECT_THAT(collect_elims(cdcl.constrain(&rl2)), IsEmpty());
}

TEST_F(CdclEliminationGeneratorUnitTest, ThreeMemberAvoidanceConstrainMiddleMemberYieldsLast) {
    cdcl.learn(make_lemma({&rl0, &rl1, &rl2}));

    EXPECT_THAT(collect_elims(cdcl.constrain(&rl0)), IsEmpty());

    auto from_middle = collect_elims(cdcl.constrain(&rl1));
    EXPECT_THAT(from_middle, ElementsAre(&rl2));
}

// ---------------------------------------------------------------------------
// constrain — batch of learn + multiple constrain in one scenario
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, LearnManyAvoidancesThenConstrainOneResolutionPerGoal) {
    cdcl.learn(make_lemma({&rl0, &rl1}));
    cdcl.learn(make_lemma({&rl2, &rl3}));
    cdcl.learn(make_lemma({&rl0_alt, &rl2}));
    cdcl.learn(make_lemma({&rl_root, &rl_child}));

    auto e_root = collect_elims(cdcl.constrain(&rl_root));
    auto e_gl0 = collect_elims(cdcl.constrain(&rl0_alt));
    auto e_gl2 = collect_elims(cdcl.constrain(&rl2));

    EXPECT_THAT(e_root, ElementsAre(&rl_child));
    EXPECT_THAT(e_gl0, ElementsAre(&rl2));
    EXPECT_THAT(e_gl2, ElementsAre(&rl3));
}
