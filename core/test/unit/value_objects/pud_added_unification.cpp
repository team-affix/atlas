// pud_added_unification: ordering on (var_idx, value pointer).

#include <gtest/gtest.h>
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/expr.hpp"

struct PudAddedUnificationTest : public ::testing::Test {
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
};

TEST_F(PudAddedUnificationTest, EqualWhenVarIdxAndValueMatch) {
    const pud_added_unification left{0, &var0};
    const pud_added_unification right{0, &var0};
    EXPECT_EQ(left, right);
}

TEST_F(PudAddedUnificationTest, OrdersByVarIdxThenValue) {
    const pud_added_unification early{0, &var0};
    const pud_added_unification later_idx{1, &var0};
    EXPECT_LT(early, later_idx);

    const pud_added_unification later_value{0, &var1};
    EXPECT_NE(early, later_value);
}
