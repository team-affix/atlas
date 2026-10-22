#include <set>
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
using frontier_t = std::set<pud_derivation_id>;

struct MockAddConjecture {
    MOCK_METHOD(pud_frontier_change, add_conjecture, (const expr*, const initial_goal_exprs&, uint32_t));
};
struct MockExpand {
    MOCK_METHOD(pud_frontier_change, expand, (pud_derivation_id, pud_body_goal_id));
};
struct MockChooseExpansion {
    MOCK_METHOD(pud_expansion_choice, choose_expansion, ((const frontier_t&)));
};

using test_solver_t = pud_random_solver<
    NiceMock<MockAddConjecture>, NiceMock<MockExpand>, NiceMock<MockChooseExpansion>>;

struct PudRandomSolverTest : public ::testing::Test {
    NiceMock<MockAddConjecture> add_conjecture;
    NiceMock<MockExpand> expand;
    NiceMock<MockChooseExpansion> choose_expansion;
    test_solver_t solver{add_conjecture, expand, choose_expansion};
    initial_goal_exprs goals;
    expr head{expr::var{0}};

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
    EXPECT_CALL(choose_expansion, choose_expansion(_)).Times(0);
    coroutine<pud_solver_step, void> solving = solver.solve();
    solving.resume();
    EXPECT_FALSE(solving.has_yield());
    EXPECT_TRUE(solving.done());
}

TEST_F(PudRandomSolverTest, ProvenDerivationIsReportedForItsConjecture) {
    start(pud_frontier_change{.opened = {}, .proven = {pud_derivation_id{5}}, .refuted = {}});
    coroutine<pud_solver_step, void> solving = solver.solve();
    EXPECT_EQ(next(solving), pud_solver_step{(pud_solution{0, pud_derivation_id{5}})});
    solving.resume();
    EXPECT_TRUE(solving.done());
}

TEST_F(PudRandomSolverTest, SolutionsAreReportedBeforeAnyDecision) {
    start(pud_frontier_change{
        .opened = {pud_derivation_id{3}}, .proven = {pud_derivation_id{4}}, .refuted = {}});
    coroutine<pud_solver_step, void> solving = solver.solve();
    EXPECT_CALL(choose_expansion, choose_expansion(_)).Times(0);
    EXPECT_TRUE(std::holds_alternative<pud_solution>(next(solving)));
}

TEST_F(PudRandomSolverTest, SolutionsAreReportedOldestFirst) {
    start(pud_frontier_change{
        .opened = {}, .proven = {pud_derivation_id{9}, pud_derivation_id{4}}, .refuted = {}});
    coroutine<pud_solver_step, void> solving = solver.solve();
    EXPECT_EQ(std::get<pud_solution>(next(solving)).derivation, pud_derivation_id{4});
    EXPECT_EQ(std::get<pud_solution>(next(solving)).derivation, pud_derivation_id{9});
}

TEST_F(PudRandomSolverTest, EveryOpenDerivationIsOffered) {
    start(pud_frontier_change{
        .opened = {pud_derivation_id{3}, pud_derivation_id{6}}, .proven = {}, .refuted = {}});
    EXPECT_CALL(choose_expansion, choose_expansion(frontier_t{pud_derivation_id{3}, pud_derivation_id{6}}))
        .WillOnce(Return(pud_expansion_choice{pud_derivation_id{3}, 2}));
    coroutine<pud_solver_step, void> solving = solver.solve();
    solving.resume();
}

TEST_F(PudRandomSolverTest, ChosenUnfoldIsUnfoldedAndThenYielded) {
    start(pud_frontier_change{.opened = {pud_derivation_id{3}}, .proven = {}, .refuted = {}});
    ON_CALL(choose_expansion, choose_expansion(_)).WillByDefault(Return(pud_expansion_choice{pud_derivation_id{3}, 2}));
    InSequence in_order;
    EXPECT_CALL(choose_expansion, choose_expansion(_));
    EXPECT_CALL(expand, expand(pud_derivation_id{3}, 2u)).WillOnce(Return(pud_frontier_change{}));
    coroutine<pud_solver_step, void> solving = solver.solve();
    EXPECT_EQ(next(solving), pud_solver_step{(pud_expansion_choice{pud_derivation_id{3}, 2})});
}

TEST_F(PudRandomSolverTest, UnfoldedDerivationLeavesTheFrontier) {
    start(pud_frontier_change{.opened = {pud_derivation_id{3}}, .proven = {}, .refuted = {}});
    ON_CALL(choose_expansion, choose_expansion(_)).WillByDefault(Return(pud_expansion_choice{pud_derivation_id{3}, 0}));
    EXPECT_CALL(expand, expand(_, _)).WillOnce(Return(pud_frontier_change{}));
    coroutine<pud_solver_step, void> solving = solver.solve();
    next(solving);
    solving.resume();
    EXPECT_TRUE(solving.done());
}

TEST_F(PudRandomSolverTest, OpenedChildrenJoinTheFrontier) {
    start(pud_frontier_change{.opened = {pud_derivation_id{3}}, .proven = {}, .refuted = {}});
    EXPECT_CALL(choose_expansion, choose_expansion(_)).WillOnce(Return(pud_expansion_choice{pud_derivation_id{3}, 0}));
    EXPECT_CALL(expand, expand(pud_derivation_id{3}, 0u))
        .WillOnce(Return(pud_frontier_change{.opened = {pud_derivation_id{7}}, .proven = {}, .refuted = {}}));
    coroutine<pud_solver_step, void> solving = solver.solve();
    next(solving);
    EXPECT_CALL(choose_expansion, choose_expansion(frontier_t{pud_derivation_id{7}}))
        .WillOnce(Return(pud_expansion_choice{pud_derivation_id{7}, 1}));
    EXPECT_CALL(expand, expand(pud_derivation_id{7}, 1u)).WillOnce(Return(pud_frontier_change{}));
    next(solving);
}

TEST_F(PudRandomSolverTest, ProvenChildrenAreSolutionsOfTheUnfoldedDerivationsConjecture) {
    start(pud_frontier_change{});
    start(pud_frontier_change{.opened = {pud_derivation_id{3}}, .proven = {}, .refuted = {}});
    ON_CALL(choose_expansion, choose_expansion(_)).WillByDefault(Return(pud_expansion_choice{pud_derivation_id{3}, 0}));
    EXPECT_CALL(expand, expand(_, _))
        .WillOnce(Return(pud_frontier_change{.opened = {}, .proven = {pud_derivation_id{9}}, .refuted = {}}));
    coroutine<pud_solver_step, void> solving = solver.solve();
    next(solving);
    EXPECT_EQ(next(solving), pud_solver_step{(pud_solution{1, pud_derivation_id{9}})});
}

TEST_F(PudRandomSolverTest, RefutedDerivationsLeaveTheFrontier) {
    start(pud_frontier_change{
        .opened = {pud_derivation_id{3}, pud_derivation_id{4}}, .proven = {}, .refuted = {}});
    ON_CALL(choose_expansion, choose_expansion(_)).WillByDefault(Return(pud_expansion_choice{pud_derivation_id{3}, 0}));
    EXPECT_CALL(expand, expand(_, _))
        .WillOnce(Return(pud_frontier_change{.opened = {}, .proven = {}, .refuted = {pud_derivation_id{4}}}));
    coroutine<pud_solver_step, void> solving = solver.solve();
    next(solving);
    solving.resume();
    EXPECT_TRUE(solving.done());
}

TEST_F(PudRandomSolverTest, RefutedProvenDerivationIsNeverReported) {
    start(pud_frontier_change{.opened = {}, .proven = {pud_derivation_id{5}}, .refuted = {}});
    start(pud_frontier_change{.opened = {}, .proven = {}, .refuted = {pud_derivation_id{5}}});
    coroutine<pud_solver_step, void> solving = solver.solve();
    solving.resume();
    EXPECT_TRUE(solving.done());
}

TEST_F(PudRandomSolverTest, RefutationsOfDerivationsItNeverTrackedAreIgnored) {
    start(pud_frontier_change{.opened = {pud_derivation_id{3}}, .proven = {}, .refuted = {}});
    start(pud_frontier_change{
        .opened = {}, .proven = {}, .refuted = {pud_derivation_id{70}, pud_derivation_id{71}}});
    EXPECT_CALL(choose_expansion, choose_expansion(frontier_t{pud_derivation_id{3}}))
        .WillOnce(Return(pud_expansion_choice{pud_derivation_id{3}, 0}));
    coroutine<pud_solver_step, void> solving = solver.solve();
    solving.resume();
}

