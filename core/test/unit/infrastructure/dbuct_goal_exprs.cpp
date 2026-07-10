// dbuct_goal_exprs journals set/unset per frame; pop_frame restores prior state.

#include <gtest/gtest.h>
#include "infrastructure/dbuct_goal_exprs.hpp"
#include "value_objects/lineage.hpp"

TEST(DbuctGoalExprsTest, SetAndGetWithinFrame) {
    dbuct_goal_exprs exprs;
    goal_lineage gl{nullptr, 0};
    framed_expr fe{{nullptr}, 3};

    exprs.push_frame();
    exprs.set(&gl, fe);
    EXPECT_EQ(exprs.get(&gl).frame_offset, 3u);
}

TEST(DbuctGoalExprsTest, PopRevertsSet) {
    dbuct_goal_exprs exprs;
    goal_lineage gl{nullptr, 0};
    framed_expr fe{{nullptr}, 3};

    exprs.push_frame();
    exprs.set(&gl, fe);
    exprs.pop_frame();
    EXPECT_THROW(exprs.get(&gl), std::out_of_range);
}

TEST(DbuctGoalExprsTest, PopRevertsUnset) {
    dbuct_goal_exprs exprs;
    goal_lineage gl{nullptr, 0};
    framed_expr fe{{nullptr}, 1};

    exprs.push_frame();
    exprs.set(&gl, fe);
    exprs.push_frame();
    exprs.unset(&gl);
    exprs.pop_frame();
    EXPECT_EQ(exprs.get(&gl).frame_offset, 1u);
}
