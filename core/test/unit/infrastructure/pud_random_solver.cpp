#include <map>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_random_solver.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Return;

namespace {
using frontier_t = std::map<pud_derivation_id, std::vector<pud_body_goal_id>>;

struct MockAddConjecture {
    MOCK_METHOD(pud_frontier_change, add_conjecture, (const expr*, const initial_goal_exprs&, uint32_t));
};
struct MockUnfold {
    MOCK_METHOD(pud_frontier_change, unfold, (pud_derivation_id, pud_body_goal_id));
};
struct MockGetHeadExpr {
    MOCK_METHOD(const expr*, head_expr, (pud_derivation_id));
};
struct MockChooseUnfold {
    MOCK_METHOD(pud_unfold_choice, choose_unfold, ((const frontier_t&)));
};

using test_solver_t = pud_random_solver<
    NiceMock<MockAddConjecture>, NiceMock<MockUnfold>, NiceMock<MockGetHeadExpr>, NiceMock<MockChooseUnfold>>;

pud_open_derivation open_derivation(uint32_t derivation, std::vector<pud_body_goal_id> goals) {
    pud_open_derivation opened{.derivation = pud_derivation_id{derivation}, .goals = {}};
    for (pud_body_goal_id goal : goals)
        opened.goals.push_back(pud_open_goal{.goal = goal, .candidate_count = 2});
    return opened;
}

struct PudRandomSolverTest : public ::testing::Test {
    NiceMock<MockAddConjecture> add_conjecture;
    NiceMock<MockUnfold> unfold;
    NiceMock<MockGetHeadExpr> get_head_expr;
    NiceMock<MockChooseUnfold> choose_unfold;
    test_solver_t solver{add_conjecture, unfold, get_head_expr, choose_unfold};
    initial_goal_exprs goals;
    expr head{expr::var{0}};
    expr answer{expr::var{1}};

    size_t start(pud_frontier_change change) {
        EXPECT_CALL(add_conjecture, add_conjecture(_, _, _)).WillOnce(Return(change));
        return solver.add_conjecture(&head, goals, 1);
    }

    static pud_solver_step next(coroutine<pud_solver_step, void>& solving) {
        solving.resume();
        EXPECT_TRUE(solving.has_yield());
        return solving.consume_yield();
    }
};

}

TEST_F(PudRandomSolverTest, ConjecturesAreNumberedInTheOrderTheyAreAdded) {
    EXPECT_EQ(start(pud_frontier_change{}), 0u);
    EXPECT_EQ(start(pud_frontier_change{}), 1u);
}

TEST_F(PudRandomSolverTest, AddingAConjecturePassesItsArgumentsThrough) {
    EXPECT_CALL(add_conjecture, add_conjecture(&head, Ref(goals), 4u)).WillOnce(Return(pud_frontier_change{}));
    solver.add_conjecture(&head, goals, 4);
}

TEST_F(PudRandomSolverTest, NothingOpenEndsSolvingWithoutAChoice) {
    EXPECT_CALL(choose_unfold, choose_unfold(_)).Times(0);
    coroutine<pud_solver_step, void> solving = solver.solve();
    solving.resume();
    EXPECT_FALSE(solving.has_yield());
    EXPECT_TRUE(solving.done());
}

TEST_F(PudRandomSolverTest, ProvenDerivationIsReportedWithItsAnswer) {
    start(pud_frontier_change{.opened = {}, .proven = {pud_derivation_id{5}}, .refuted = {}, .made_unit = {}});
    EXPECT_CALL(get_head_expr, head_expr(pud_derivation_id{5})).WillOnce(Return(&answer));
    coroutine<pud_solver_step, void> solving = solver.solve();
    EXPECT_EQ(next(solving), pud_solver_step{(pud_solution{0, pud_derivation_id{5}, &answer})});
    solving.resume();
    EXPECT_TRUE(solving.done());
}

TEST_F(PudRandomSolverTest, AnswerIsOnlyReadWhenReported) {
    start(pud_frontier_change{.opened = {}, .proven = {pud_derivation_id{5}}, .refuted = {}, .made_unit = {}});
    EXPECT_CALL(get_head_expr, head_expr(_)).Times(0);
    coroutine<pud_solver_step, void> solving = solver.solve();
}

TEST_F(PudRandomSolverTest, SolutionsAreReportedBeforeAnyDecision) {
    start(pud_frontier_change{
        .opened = {open_derivation(3, {0})}, .proven = {pud_derivation_id{4}}, .refuted = {}, .made_unit = {}});
    coroutine<pud_solver_step, void> solving = solver.solve();
    EXPECT_CALL(choose_unfold, choose_unfold(_)).Times(0);
    EXPECT_TRUE(std::holds_alternative<pud_solution>(next(solving)));
}

TEST_F(PudRandomSolverTest, SolutionsAreReportedOldestFirst) {
    start(pud_frontier_change{
        .opened = {}, .proven = {pud_derivation_id{9}, pud_derivation_id{4}}, .refuted = {}, .made_unit = {}});
    coroutine<pud_solver_step, void> solving = solver.solve();
    EXPECT_EQ(std::get<pud_solution>(next(solving)).derivation, pud_derivation_id{4});
    EXPECT_EQ(std::get<pud_solution>(next(solving)).derivation, pud_derivation_id{9});
}

TEST_F(PudRandomSolverTest, EveryOpenDerivationIsOfferedWithItsGoalIds) {
    start(pud_frontier_change{
        .opened = {open_derivation(3, {0, 2}), open_derivation(6, {1})}, .proven = {}, .refuted = {}, .made_unit = {}});
    EXPECT_CALL(choose_unfold, choose_unfold(frontier_t{{pud_derivation_id{3}, {0, 2}}, {pud_derivation_id{6}, {1}}}))
        .WillOnce(Return(pud_unfold_choice{pud_derivation_id{3}, 2}));
    coroutine<pud_solver_step, void> solving = solver.solve();
    solving.resume();
}

TEST_F(PudRandomSolverTest, ChosenUnfoldIsUnfoldedAndThenYielded) {
    start(pud_frontier_change{.opened = {open_derivation(3, {0, 2})}, .proven = {}, .refuted = {}, .made_unit = {}});
    ON_CALL(choose_unfold, choose_unfold(_)).WillByDefault(Return(pud_unfold_choice{pud_derivation_id{3}, 2}));
    InSequence in_order;
    EXPECT_CALL(choose_unfold, choose_unfold(_));
    EXPECT_CALL(unfold, unfold(pud_derivation_id{3}, 2u)).WillOnce(Return(pud_frontier_change{}));
    coroutine<pud_solver_step, void> solving = solver.solve();
    EXPECT_EQ(next(solving), pud_solver_step{(pud_unfold_choice{pud_derivation_id{3}, 2})});
}

TEST_F(PudRandomSolverTest, UnfoldedDerivationLeavesTheFrontier) {
    start(pud_frontier_change{.opened = {open_derivation(3, {0})}, .proven = {}, .refuted = {}, .made_unit = {}});
    ON_CALL(choose_unfold, choose_unfold(_)).WillByDefault(Return(pud_unfold_choice{pud_derivation_id{3}, 0}));
    EXPECT_CALL(unfold, unfold(_, _)).WillOnce(Return(pud_frontier_change{}));
    coroutine<pud_solver_step, void> solving = solver.solve();
    next(solving);
    solving.resume();
    EXPECT_TRUE(solving.done());
}

TEST_F(PudRandomSolverTest, OpenedChildrenJoinTheFrontier) {
    start(pud_frontier_change{.opened = {open_derivation(3, {0, 1})}, .proven = {}, .refuted = {}, .made_unit = {}});
    EXPECT_CALL(choose_unfold, choose_unfold(_)).WillOnce(Return(pud_unfold_choice{pud_derivation_id{3}, 0}));
    EXPECT_CALL(unfold, unfold(pud_derivation_id{3}, 0u))
        .WillOnce(Return(pud_frontier_change{.opened = {open_derivation(7, {1})}, .proven = {}, .refuted = {}, .made_unit = {}}));
    coroutine<pud_solver_step, void> solving = solver.solve();
    next(solving);
    EXPECT_CALL(choose_unfold, choose_unfold(frontier_t{{pud_derivation_id{7}, {1}}}))
        .WillOnce(Return(pud_unfold_choice{pud_derivation_id{7}, 1}));
    EXPECT_CALL(unfold, unfold(pud_derivation_id{7}, 1u)).WillOnce(Return(pud_frontier_change{}));
    next(solving);
}

TEST_F(PudRandomSolverTest, ProvenChildrenAreSolutionsOfTheUnfoldedDerivationsConjecture) {
    start(pud_frontier_change{});
    start(pud_frontier_change{.opened = {open_derivation(3, {0})}, .proven = {}, .refuted = {}, .made_unit = {}});
    ON_CALL(choose_unfold, choose_unfold(_)).WillByDefault(Return(pud_unfold_choice{pud_derivation_id{3}, 0}));
    EXPECT_CALL(unfold, unfold(_, _))
        .WillOnce(Return(pud_frontier_change{.opened = {}, .proven = {pud_derivation_id{9}}, .refuted = {}, .made_unit = {}}));
    ON_CALL(get_head_expr, head_expr(pud_derivation_id{9})).WillByDefault(Return(&answer));
    coroutine<pud_solver_step, void> solving = solver.solve();
    next(solving);
    EXPECT_EQ(next(solving), pud_solver_step{(pud_solution{1, pud_derivation_id{9}, &answer})});
}

TEST_F(PudRandomSolverTest, RefutedDerivationsLeaveTheFrontier) {
    start(pud_frontier_change{
        .opened = {open_derivation(3, {0}), open_derivation(4, {0})}, .proven = {}, .refuted = {}, .made_unit = {}});
    ON_CALL(choose_unfold, choose_unfold(_)).WillByDefault(Return(pud_unfold_choice{pud_derivation_id{3}, 0}));
    EXPECT_CALL(unfold, unfold(_, _))
        .WillOnce(Return(pud_frontier_change{.opened = {}, .proven = {}, .refuted = {pud_derivation_id{4}}, .made_unit = {}}));
    coroutine<pud_solver_step, void> solving = solver.solve();
    next(solving);
    solving.resume();
    EXPECT_TRUE(solving.done());
}

TEST_F(PudRandomSolverTest, RefutedProvenDerivationIsNeverReported) {
    start(pud_frontier_change{.opened = {}, .proven = {pud_derivation_id{5}}, .refuted = {}, .made_unit = {}});
    start(pud_frontier_change{.opened = {}, .proven = {}, .refuted = {pud_derivation_id{5}}, .made_unit = {}});
    EXPECT_CALL(get_head_expr, head_expr(_)).Times(0);
    coroutine<pud_solver_step, void> solving = solver.solve();
    solving.resume();
    EXPECT_TRUE(solving.done());
}

TEST_F(PudRandomSolverTest, RefutationsOfDerivationsItNeverTrackedAreIgnored) {
    start(pud_frontier_change{.opened = {open_derivation(3, {0})}, .proven = {}, .refuted = {}, .made_unit = {}});
    start(pud_frontier_change{
        .opened = {}, .proven = {}, .refuted = {pud_derivation_id{70}, pud_derivation_id{71}}, .made_unit = {}});
    EXPECT_CALL(choose_unfold, choose_unfold(frontier_t{{pud_derivation_id{3}, {0}}}))
        .WillOnce(Return(pud_unfold_choice{pud_derivation_id{3}, 0}));
    coroutine<pud_solver_step, void> solving = solver.solve();
    solving.resume();
}

TEST_F(PudRandomSolverTest, MadeUnitGoalsChangeNoDecision) {
    start(pud_frontier_change{.opened = {open_derivation(3, {0, 1})}, .proven = {}, .refuted = {}, .made_unit = {}});
    start(pud_frontier_change{
        .opened = {}, .proven = {}, .refuted = {}, .made_unit = {pud_unit_goal{pud_derivation_id{3}, 1}}});
    EXPECT_CALL(choose_unfold, choose_unfold(frontier_t{{pud_derivation_id{3}, {0, 1}}}))
        .WillOnce(Return(pud_unfold_choice{pud_derivation_id{3}, 0}));
    coroutine<pud_solver_step, void> solving = solver.solve();
    solving.resume();
}
