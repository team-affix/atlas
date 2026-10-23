#include <optional>
#include <set>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_solver.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockAddConjecture {
    MOCK_METHOD(pud_node_id, add_conjecture, (const expr*, const std::vector<const expr*>&, uint32_t));
};
struct MockOpenRoots {
    MOCK_METHOD(pud_frontier_change, open, ());
};
struct MockExpand {
    MOCK_METHOD(pud_frontier_change, expand, (pud_node_id, pud_body_goal_id));
};
struct MockFindUnitUnfold {
    MOCK_METHOD(std::optional<pud_unfold>, find_unit_unfold, ());
};
struct MockChooseExpansion {
    MOCK_METHOD(pud_unfold, choose_expansion, (const std::set<pud_node_id>&));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_node_id));
};
struct MockMaterialize {
    MOCK_METHOD(const expr*, materialize, (pud_node_id, const expr*));
};

using test_solver_t = pud_solver<
    NiceMock<MockAddConjecture>, NiceMock<MockOpenRoots>, NiceMock<MockExpand>, NiceMock<MockFindUnitUnfold>,
    NiceMock<MockChooseExpansion>, NiceMock<MockGetNodeGoals>, NiceMock<MockMaterialize>>;

struct PudSolverTest : public ::testing::Test {
    NiceMock<MockAddConjecture> add_conjecture;
    NiceMock<MockOpenRoots> open_roots;
    NiceMock<MockExpand> expander;
    NiceMock<MockFindUnitUnfold> units;
    NiceMock<MockChooseExpansion> chooser;
    NiceMock<MockGetNodeGoals> node_goals;
    NiceMock<MockMaterialize> materializer;
    test_solver_t solver{add_conjecture, open_roots, expander, units, chooser, node_goals, materializer};
    expr head{expr::functor{1, {}}};
    expr answer{expr::functor{2, {}}};
    expr open_goal{expr::functor{3, {}}};
    pud_goal_list no_goals{.goals = {}, .next_id = 0};
    pud_goal_list has_goals = pud_goal_list{.goals = {}, .next_id = 0}.adding({&open_goal});
    std::optional<coroutine<pud_solver_step, void>> solving;

    void SetUp() override {
        ON_CALL(node_goals, get(_)).WillByDefault(ReturnRef(no_goals));
        ON_CALL(node_goals, get(5)).WillByDefault(ReturnRef(has_goals));
        ON_CALL(node_goals, get(6)).WillByDefault(ReturnRef(has_goals));
        ON_CALL(node_goals, get(12)).WillByDefault(ReturnRef(has_goals));
    }

    pud_node_id add(pud_node_id root, pud_frontier_change change) {
        ON_CALL(add_conjecture, add_conjecture(_, _, _)).WillByDefault(Return(root));
        ON_CALL(open_roots, open()).WillByDefault(Return(change));
        return solver.add_conjecture(&head, {}, 0);
    }

    std::optional<pud_solver_step> step() {
        if (!solving.has_value())
            solving.emplace(solver.solve());
        solving->resume();
        if (!solving->has_yield())
            return std::nullopt;
        return solving->consume_yield();
    }
};

}

TEST_F(PudSolverTest, AConjectureIsNamedByItsRoot) {
    EXPECT_EQ(add(5, pud_frontier_change{.offspring = {5}, .refuted = {}}), 5u);
}

TEST_F(PudSolverTest, AProvenLeafIsReportedWithTheHeadReadInItsBindings) {
    ON_CALL(node_goals, get(5)).WillByDefault(ReturnRef(no_goals));
    add(5, pud_frontier_change{.offspring = {5}, .refuted = {}});
    ON_CALL(materializer, materialize(5, &head)).WillByDefault(Return(&answer));
    EXPECT_EQ(step(), (pud_solver_step{pud_solution{.conjecture = 5, .leaf = 5, .answer = &answer}}));
    EXPECT_TRUE(solver.exhausted(5));
    EXPECT_FALSE(step().has_value());
}

TEST_F(PudSolverTest, AForcedUnfoldIsTakenBeforeAnyChoice) {
    add(5, pud_frontier_change{.offspring = {5}, .refuted = {}});
    ON_CALL(units, find_unit_unfold()).WillByDefault(Return(pud_unfold{.leaf = 9, .goal = 2}));
    EXPECT_CALL(chooser, choose_expansion(_)).Times(0);
    EXPECT_CALL(expander, expand(9, 2));
    EXPECT_EQ(step(), (pud_solver_step{pud_unfold{.leaf = 9, .goal = 2}}));
    EXPECT_FALSE(solver.exhausted(5));
}

TEST_F(PudSolverTest, WithoutAForcedUnfoldAFrontierLeafIsChosen) {
    add(5, pud_frontier_change{.offspring = {5}, .refuted = {}});
    EXPECT_CALL(chooser, choose_expansion(std::set<pud_node_id>{5})).WillOnce(Return(pud_unfold{.leaf = 5, .goal = 0}));
    EXPECT_CALL(expander, expand(5, 0)).WillOnce(Return(pud_frontier_change{.offspring = {6, 7}, .refuted = {}}));
    step();
    EXPECT_FALSE(solver.exhausted(5));
    ON_CALL(materializer, materialize(7, &head)).WillByDefault(Return(&answer));
    EXPECT_EQ(step(), (pud_solver_step{pud_solution{.conjecture = 5, .leaf = 7, .answer = &answer}}));
}

TEST_F(PudSolverTest, ExpandingAnAxiomLeafAddsNothingToAConjecture) {
    add(5, pud_frontier_change{.offspring = {5}, .refuted = {}});
    EXPECT_CALL(units, find_unit_unfold()).WillOnce(Return(pud_unfold{.leaf = 1, .goal = 0})).WillRepeatedly(Return(std::nullopt));
    ON_CALL(expander, expand(1, 0)).WillByDefault(Return(pud_frontier_change{.offspring = {12, 13}, .refuted = {}}));
    step();
    EXPECT_CALL(chooser, choose_expansion(std::set<pud_node_id>{5})).WillOnce(Return(pud_unfold{.leaf = 5, .goal = 0}));
    step();
}

TEST_F(PudSolverTest, RefutedLeavesLeaveTheirConjectureAndAnEmptyConjectureIsExhausted) {
    add(5, pud_frontier_change{.offspring = {5}, .refuted = {}});
    ON_CALL(chooser, choose_expansion(_)).WillByDefault(Return(pud_unfold{.leaf = 5, .goal = 0}));
    ON_CALL(expander, expand(5, 0)).WillByDefault(Return(pud_frontier_change{.offspring = {}, .refuted = {6, 7}}));
    step();
    EXPECT_TRUE(solver.exhausted(5));
    EXPECT_FALSE(step().has_value());
}
