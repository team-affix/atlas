#include <gtest/gtest.h>
#include "pud_expr_structure.hpp"
#include "value_objects/expr.hpp"

// The structural equality other tests compare terms from different pools with, checked on
// hand-built terms that are never shared, so pointer equality never decides a case.
TEST(SameStructureTest, AVariableMatchesAVariableOfTheSameIndex) {
    const expr left{expr::var{3}};
    const expr right{expr::var{3}};
    EXPECT_TRUE(same_structure(&left, &right));
}

TEST(SameStructureTest, VariablesOfDifferentIndicesDiffer) {
    const expr left{expr::var{3}};
    const expr right{expr::var{4}};
    EXPECT_FALSE(same_structure(&left, &right));
}

TEST(SameStructureTest, AVariableNeverMatchesAFunctor) {
    const expr variable{expr::var{0}};
    const expr atom{expr::functor{0, {}}};
    EXPECT_FALSE(same_structure(&variable, &atom));
    EXPECT_FALSE(same_structure(&atom, &variable));
}

TEST(SameStructureTest, DistinctCopiesOfOneTermMatch) {
    const expr left_a{expr::functor{1, {}}};
    const expr left_x{expr::var{2}};
    const expr left{expr::functor{5, {&left_a, &left_x}}};
    const expr right_a{expr::functor{1, {}}};
    const expr right_x{expr::var{2}};
    const expr right{expr::functor{5, {&right_a, &right_x}}};
    EXPECT_TRUE(same_structure(&left, &right));
}

TEST(SameStructureTest, FunctorsWithDifferentIdsDiffer) {
    const expr left{expr::functor{1, {}}};
    const expr right{expr::functor{2, {}}};
    EXPECT_FALSE(same_structure(&left, &right));
}

TEST(SameStructureTest, FunctorsWithDifferentAritiesDiffer) {
    const expr arg{expr::functor{1, {}}};
    const expr left{expr::functor{5, {&arg}}};
    const expr right{expr::functor{5, {&arg, &arg}}};
    EXPECT_FALSE(same_structure(&left, &right));
}

TEST(SameStructureTest, ADifferenceDeepInsideAnArgumentIsFound) {
    const expr left_leaf{expr::var{0}};
    const expr left_inner{expr::functor{2, {&left_leaf}}};
    const expr left{expr::functor{3, {&left_inner}}};
    const expr right_leaf{expr::var{1}};
    const expr right_inner{expr::functor{2, {&right_leaf}}};
    const expr right{expr::functor{3, {&right_inner}}};
    EXPECT_FALSE(same_structure(&left, &right));
}

TEST(SameStructureTest, ArgumentOrderMatters) {
    const expr a{expr::functor{1, {}}};
    const expr b{expr::functor{2, {}}};
    const expr left{expr::functor{5, {&a, &b}}};
    const expr right{expr::functor{5, {&b, &a}}};
    EXPECT_FALSE(same_structure(&left, &right));
}
