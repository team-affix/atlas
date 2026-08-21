// framed_expr: the defaulted operator<=>, which every framed store keys on.
//
// A framed_expr is (skeleton, frame_offset). The same interned skeleton
// instantiated in a different resolution frame is a DIFFERENT term, so the
// ordering has to separate on frame_offset even when the skeleton is shared --
// otherwise a recursive rule's fresh copy of a variable collapses onto the
// parent's binding slot and the two get unified with each other.

#include <array>
#include <gtest/gtest.h>
#include "value_objects/framed_expr.hpp"
#include "value_objects/expr.hpp"

struct FramedExprTest : public ::testing::Test {
    // Held in one array so the two skeleton pointers have a defined relative order.
    std::array<expr, 2> skeletons{expr{expr::var{0}}, expr{expr::var{1}}};

    static constexpr uint32_t kFrameEarly = 0;
    static constexpr uint32_t kFrameLate = 7;
};

TEST_F(FramedExprTest, OrderingBySkeletonThenFrameOffset) {
    const framed_expr early{&skeletons[0], kFrameEarly};
    const framed_expr same_as_early{&skeletons[0], kFrameEarly};
    EXPECT_EQ(early, same_as_early);
    EXPECT_FALSE(early < same_as_early);
    EXPECT_FALSE(same_as_early < early);

    // Same skeleton, later frame: strictly greater, never equivalent.
    const framed_expr late{&skeletons[0], kFrameLate};
    EXPECT_NE(early, late);
    EXPECT_LT(early, late);

    // The skeleton dominates: a lower skeleton sorts first whatever the frame is.
    const framed_expr other_skeleton{&skeletons[1], kFrameEarly};
    EXPECT_LT(early, other_skeleton);
    EXPECT_LT(late, other_skeleton);
}
