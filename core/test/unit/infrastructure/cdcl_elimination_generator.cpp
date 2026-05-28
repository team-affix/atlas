// cdcl_elimination_generator learns pairwise avoidance lemmas and yields eliminations
// during constrain. Unit tests mock i_trail logging and assert learn/constrain outcomes
// on independent and dependent lineage trees.

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
using ::testing::UnorderedElementsAre;
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
// Lineage naming: lin_i = ith root goal; lin_i_j = jth resolution of goal i;
// lin_i_j_k = kth subgoal (goal) of resolution j of goal i; lin_i_j_k_m = mth resolution of that subgoal; ...
struct CdclEliminationGeneratorUnitTest : public ::testing::Test {
protected:
    NiceMock<MockTrail> trail;
    cdcl_elimination_generator cdcl{trail};

    // Four independent root goals (no derivation parent).
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

    goal_lineage lin_0{nullptr, &goal_expr0};
    goal_lineage lin_1{nullptr, &goal_expr1};
    goal_lineage lin_2{nullptr, &goal_expr2};
    goal_lineage lin_3{nullptr, &goal_expr3};

    resolution_lineage lin_0_0{&lin_0, &rule0};
    resolution_lineage lin_0_1{&lin_0, &rule0_alt};
    resolution_lineage lin_1_0{&lin_1, &rule1};
    resolution_lineage lin_2_0{&lin_2, &rule2};
    resolution_lineage lin_3_0{&lin_3, &rule3};

    // Dependent tree (one resolution lin_4_0 at goal 4, then sibling subgoals):
    //   lin_4 --lin_4_0--> lin_4_0_0 --lin_4_0_0_0-->
    //                    \-> lin_4_0_1 --lin_4_0_1_0-->
    expr goal_expr4{expr::var{20}};
    expr goal_expr4_0_0{expr::var{21}};
    expr goal_expr4_0_1{expr::var{22}};
    expr head4_0{expr::var{30}};
    expr head4_0_0_0{expr::var{31}};
    expr head4_0_1_0{expr::var{33}};

    rule rule4_0{&head4_0, {}};
    rule rule4_0_0_0{&head4_0_0_0, {}};
    rule rule4_0_1_0{&head4_0_1_0, {}};

    goal_lineage lin_4{nullptr, &goal_expr4};
    resolution_lineage lin_4_0{&lin_4, &rule4_0};
    goal_lineage lin_4_0_0{&lin_4_0, &goal_expr4_0_0};
    goal_lineage lin_4_0_1{&lin_4_0, &goal_expr4_0_1};
    resolution_lineage lin_4_0_0_0{&lin_4_0_0, &rule4_0_0_0};
    resolution_lineage lin_4_0_1_0{&lin_4_0_1, &rule4_0_1_0};
};

// ---------------------------------------------------------------------------
// learn
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, LearnUnitAvoidanceReturnsEliminationWithoutStoring) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0})), std::optional{&lin_0_0});
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnMultiMemberAvoidanceReturnsNull) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnUnitAvoidanceDoesNotLogToTrail) {
    StrictMock<MockTrail> strict_trail;
    cdcl_elimination_generator strict_cdcl{strict_trail};

    EXPECT_CALL(strict_trail, log(_)).Times(0);
    EXPECT_EQ(strict_cdcl.learn(make_lemma({&lin_0_0})), std::optional{&lin_0_0});
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnMultiMemberAvoidanceLogsToTrail) {
    StrictMock<MockTrail> strict_trail;
    cdcl_elimination_generator strict_cdcl{strict_trail};

    EXPECT_CALL(strict_trail, log(_)).Times(AtLeast(1));
    EXPECT_EQ(strict_cdcl.learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnDuplicateAvoidanceIsIdempotent) {
    const lemma l = make_lemma({&lin_0_0, &lin_1_0});
    EXPECT_EQ(cdcl.learn(l), std::nullopt);
    EXPECT_EQ(cdcl.learn(l), std::nullopt);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnDuplicateAvoidanceDoesNotLogToTrail) {
    StrictMock<MockTrail> strict_trail;
    cdcl_elimination_generator strict_cdcl{strict_trail};

    const lemma l = make_lemma({&lin_0_0, &lin_1_0});

    EXPECT_CALL(strict_trail, log(_)).Times(AtLeast(1));
    EXPECT_EQ(strict_cdcl.learn(l), std::nullopt);
    testing::Mock::VerifyAndClearExpectations(&strict_trail);

    EXPECT_CALL(strict_trail, log(_)).Times(0);
    EXPECT_EQ(strict_cdcl.learn(l), std::nullopt);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnThreeIndependentPairAvoidances) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0, &lin_1_0})), std::nullopt);
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_2_0, &lin_3_0})), std::nullopt);
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_4_0_0_0, &lin_4_0_1_0})), std::nullopt);
}

// ---------------------------------------------------------------------------
// constrain — no prior learn
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainWithNoLearnedAvoidancesYieldsNothing) {
    auto elims = collect_elims(cdcl.constrain(&lin_0_0));
    EXPECT_THAT(elims, IsEmpty());
}

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainAfterUnitLearnYieldsNothing) {
    EXPECT_EQ(cdcl.learn(make_lemma({&lin_0_0})), std::optional{&lin_0_0});
    auto elims = collect_elims(cdcl.constrain(&lin_0_0));
    EXPECT_THAT(elims, IsEmpty());
}

// ---------------------------------------------------------------------------
// constrain — single stored avoidance
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainMember1YieldsMember2InBinaryAvoidance) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));

    auto elims = collect_elims(cdcl.constrain(&lin_0_0));

    EXPECT_THAT(elims, ElementsAre(&lin_1_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainMutuallyExclusiveResolutionErasesWithoutYield) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));

    auto elims = collect_elims(cdcl.constrain(&lin_0_1));

    EXPECT_THAT(elims, IsEmpty());
}

// ---------------------------------------------------------------------------
// constrain — multiple learned avoidances (independent)
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, TwoIndependentAvoidancesConstrainIndependently) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_2_0, &lin_3_0}));

    auto from_lin_0 = collect_elims(cdcl.constrain(&lin_0_0));
    auto from_lin_2 = collect_elims(cdcl.constrain(&lin_2_0));

    EXPECT_THAT(from_lin_0, ElementsAre(&lin_1_0));
    EXPECT_THAT(from_lin_2, ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, SequentialConstrainOnDisjointIndependentAvoidances) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_2_0, &lin_3_0}));

    auto first = collect_elims(cdcl.constrain(&lin_0_0));
    auto second = collect_elims(cdcl.constrain(&lin_2_0));

    EXPECT_THAT(first, ElementsAre(&lin_1_0));
    EXPECT_THAT(second, ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainOnGoalWithNoLearnedAvoidanceYieldsNothing) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));

    auto elims = collect_elims(cdcl.constrain(&lin_3_0));

    EXPECT_THAT(elims, IsEmpty());
}

// ---------------------------------------------------------------------------
// constrain — two avoidances watching lin_0 (one constrain per goal)
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, ConstrainYieldsFromConsistentAvoidanceAndNotMutuallyExclusiveAvoidance) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_0_1, &lin_2_0}));

    auto elims = collect_elims(cdcl.constrain(&lin_0_1));

    EXPECT_THAT(elims, ElementsAre(&lin_2_0));
}

// ---------------------------------------------------------------------------
// constrain — dependent tree (distinct parents within each avoidance)
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, DependentAndIndependentAvoidancesDoNotInterfere) {
    cdcl.learn(make_lemma({&lin_4_0_0_0, &lin_4_0_1_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));

    auto from_dependent = collect_elims(cdcl.constrain(&lin_4_0_0_0));
    auto from_independent = collect_elims(cdcl.constrain(&lin_0_0));

    EXPECT_THAT(from_dependent, ElementsAre(&lin_4_0_1_0));
    EXPECT_THAT(from_independent, ElementsAre(&lin_1_0));
}

// ---------------------------------------------------------------------------
// constrain — sequential reduction of a three-member avoidance
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, ThreeMemberAvoidanceSequentialConstrainEventuallyYieldsLast) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0}));

    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_1_0)), ElementsAre(&lin_2_0));
    EXPECT_THAT(collect_elims(cdcl.constrain(&lin_2_0)), IsEmpty());
}

// ---------------------------------------------------------------------------
// constrain — batch of learn + multiple constrain in one scenario
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest, LearnManyAvoidancesThenConstrainOneResolutionPerGoal) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_2_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_0_1, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_4_0_0_0, &lin_4_0_1_0}));

    auto e_lin_4_0_0 = collect_elims(cdcl.constrain(&lin_4_0_0_0));
    auto e_lin_0 = collect_elims(cdcl.constrain(&lin_0_1));
    auto e_lin_2 = collect_elims(cdcl.constrain(&lin_2_0));

    EXPECT_THAT(e_lin_4_0_0, ElementsAre(&lin_4_0_1_0));
    EXPECT_THAT(e_lin_0, ElementsAre(&lin_2_0));
    EXPECT_THAT(e_lin_2, ElementsAre(&lin_3_0));
}

// ---------------------------------------------------------------------------
// constrain — four avoidances, each with a resolution on shared goal lin_0
//   av0: {lin_0_0, lin_1_0}   av1: {lin_0_0, lin_2_0}   av2: {lin_0_0, lin_3_0}
//   av3: {lin_0_1, lin_3_0}
// ---------------------------------------------------------------------------

TEST_F(CdclEliminationGeneratorUnitTest,
    FourAvoidancesSharingLin0ConstrainMemberLin0_0YieldsFromConsistentAvoidances) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_0_1, &lin_3_0}));

    auto elims = collect_elims(cdcl.constrain(&lin_0_0));

    EXPECT_THAT(elims, UnorderedElementsAre(&lin_1_0, &lin_2_0, &lin_3_0));
}

TEST_F(CdclEliminationGeneratorUnitTest,
    FourAvoidancesSharingLin0ConstrainMemberLin0_1YieldsFromConsistentAvoidance) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_3_0}));
    cdcl.learn(make_lemma({&lin_0_1, &lin_3_0}));

    auto elims = collect_elims(cdcl.constrain(&lin_0_1));

    EXPECT_THAT(elims, ElementsAre(&lin_3_0));
}

TEST_F(CdclEliminationGeneratorUnitTest,
    FourAvoidancesSharingLin0ConstrainExclusiveLin0_1ErasesOnlyLin0_0Avoidances) {
    cdcl.learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_2_0}));
    cdcl.learn(make_lemma({&lin_0_0, &lin_3_0}));

    auto elims = collect_elims(cdcl.constrain(&lin_0_1));

    EXPECT_THAT(elims, IsEmpty());
}

TEST_F(CdclEliminationGeneratorUnitTest,
    FourAvoidancesSharingLin0ConstrainExclusiveLin0_0ErasesOnlyLin0_1Avoidance) {
    cdcl.learn(make_lemma({&lin_0_1, &lin_3_0}));

    auto elims = collect_elims(cdcl.constrain(&lin_0_0));

    EXPECT_THAT(elims, IsEmpty());
}
