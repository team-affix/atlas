// import_goals_from_string: parse comma-separated goal atoms into i_push_initial_goal_expr.
// Invariants: goal count, var_name_to_idx map, left-to-right index assignment, parse errors throw.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include "parser_fixture.hpp"
#include "infrastructure/atom_names.hpp"
#include "parser/hpp/import_goals_from_string.hpp"
#include "infrastructure/initial_goal_exprs.hpp"

using ::testing::SizeIs;

struct ImportGoalsFromStringTest : ParserCoreFixture {};

TEST_F(ImportGoalsFromStringTest, SingleGoal) {
    initial_goal_exprs goals;

    auto var_name_to_idx = import_goals_from_string("reach(0, 2)", *pool, *pool, *var_seq, goals, atom_map, next_atom_id);
    EXPECT_EQ(goals.count(), 1u);
    EXPECT_EQ(goals.get(0), pool->make_functor(atom_map.at("reach"), {pool->make_functor(atom_map.at("0"), {}), pool->make_functor(atom_map.at("2"), {})}));
    EXPECT_TRUE(var_name_to_idx.empty());
}

TEST_F(ImportGoalsFromStringTest, MultipleGoalsShareVar) {
    initial_goal_exprs goals;

    auto var_name_to_idx = import_goals_from_string("p(X), q(X)", *pool, *pool, *var_seq, goals, atom_map, next_atom_id);
    EXPECT_EQ(goals.count(), 2u);

    const expr* x = pool->make_var(0);
    EXPECT_EQ(goals.get(0), pool->make_functor(atom_map.at("p"), {x}));
    EXPECT_EQ(goals.get(1), pool->make_functor(atom_map.at("q"), {x}));
    EXPECT_EQ(var_name_to_idx.at("X"), 0u);
}

TEST_F(ImportGoalsFromStringTest, ComplexGoals) {
    initial_goal_exprs goals;

    auto var_name_to_idx = import_goals_from_string("reach(X, Y), next(Y, Z), base(Z)", *pool, *pool, *var_seq, goals, atom_map, next_atom_id);

    EXPECT_EQ(goals.count(), 3u);
    EXPECT_THAT(var_name_to_idx, SizeIs(3));
    EXPECT_EQ(var_name_to_idx.at("X"), 0u);
    EXPECT_EQ(var_name_to_idx.at("Y"), 1u);
    EXPECT_EQ(var_name_to_idx.at("Z"), 2u);

    const expr* x = pool->make_var(0);
    const expr* y = pool->make_var(1);
    const expr* z = pool->make_var(2);

    EXPECT_EQ(goals.get(0), pool->make_functor(atom_map.at("reach"), {x, y}));
    EXPECT_EQ(goals.get(1), pool->make_functor(atom_map.at("next"), {y, z}));
    EXPECT_EQ(goals.get(2), pool->make_functor(atom_map.at("base"), {z}));
}

TEST_F(ImportGoalsFromStringTest, BadInputThrows) {
    initial_goal_exprs goals;
    EXPECT_THROW(import_goals_from_string(":-", *pool, *pool, *var_seq, goals, atom_map, next_atom_id), std::runtime_error);
}
