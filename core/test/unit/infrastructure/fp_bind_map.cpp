#include <gtest/gtest.h>
#include <deque>
#include <vector>
#include "infrastructure/fp_bind_map.hpp"
#include "infrastructure/order_maintenance.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

// ---------------------------------------------------------------------------
// Reference tree layout (allocated via order_maintenance):
//
//   root
//   ├── A
//   │   └── B
//   ├── probe   ← allocated between A and C; used for precision queries
//   └── C
//       └── D
//
// Euler-tour ordering:
//   root.open < A.open < B.open < B.close < A.close
//             < probe.open < probe.close
//             < C.open < D.open < D.close < C.close < root.close
// ---------------------------------------------------------------------------

namespace {

framed_expr make_framed(uint32_t functor_id, uint32_t frame_offset = 0) {
    // std::deque: push_back never invalidates existing references/pointers,
    // so framed_expr::skeleton pointers remain valid through all later calls.
    static std::deque<expr> exprs;
    exprs.push_back(expr{expr::functor{functor_id, {}}});
    return framed_expr{&exprs.back(), frame_offset};
}

bool same_value(const std::optional<framed_expr>& result, const framed_expr& expected) {
    return result.has_value() && *result == expected;
}

} // namespace

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

struct FpBindMapTest : public ::testing::Test {
    FpBindMapTest()
        : root_(om_.allocate_root())
        , a_(om_.allocate_child_of(root_))
        , b_(om_.allocate_child_of(a_))
        , probe_(om_.allocate_child_of(root_))
        , c_(om_.allocate_child_of(root_))
        , d_(om_.allocate_child_of(c_))
        , val1_(make_framed(1))
        , val2_(make_framed(2))
        , val3_(make_framed(3))
        , val4_(make_framed(4))
        , val5_(make_framed(5)) {}

    order_maintenance om_;
    fp_bind_map bm_;
    om_interval root_;
    om_interval a_;
    om_interval b_;
    om_interval probe_;
    om_interval c_;
    om_interval d_;

    const framed_expr val1_;
    const framed_expr val2_;
    const framed_expr val3_;
    const framed_expr val4_;
    const framed_expr val5_;

    static constexpr uint32_t k_var_x = 1;
    static constexpr uint32_t k_var_y = 2;
    static constexpr uint32_t k_var_z = 3;
};

// ---------------------------------------------------------------------------
// Unbound queries — would pass trivially if record() does nothing, but
// would fail if query() crashes or returns garbage on an empty map.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, EmptyMapReturnsNullopt) {
    EXPECT_FALSE(bm_.query(root_.open, k_var_x).has_value());
}

TEST_F(FpBindMapTest, UnrecordedVarReturnsNullopt) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    EXPECT_FALSE(bm_.query(root_.open, k_var_y).has_value());
}

// ---------------------------------------------------------------------------
// Single binding — inheritance
// Catch bug: query searches wrong direction (successor instead of predecessor).
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, QueryAtRecordingNodeReturnsValue) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_x), val1_));
}

TEST_F(FpBindMapTest, ChildInheritsAncestorBinding) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_x), val1_));
}

TEST_F(FpBindMapTest, GrandchildInheritsGrandparentBinding) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), val1_));
}

TEST_F(FpBindMapTest, StarTreeAllDescendantsInheritRoot) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(d_.open, k_var_x), val1_));
}

// ---------------------------------------------------------------------------
// Rebinding / shadowing
// Catch bug: child's record overwrites root's record globally instead of
// locally, or close event is missing/wrong.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, ChildRebindingHidesParent) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(a_.open,    a_.close,    k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_x), val2_));
}

TEST_F(FpBindMapTest, ParentUnaffectedByChildRebinding) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(a_.open,    a_.close,    k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_x), val1_));
}

TEST_F(FpBindMapTest, SiblingAfterRebindingChildSeesParent) {
    // C is a sibling of A.  Without the close event at A.close, the
    // predecessor of C.open would be A's binding, which is wrong.
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(a_.open,    a_.close,    k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
}

TEST_F(FpBindMapTest, GrandchildRebindingChain) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(a_.open,    a_.close,    k_var_x, val2_);
    bm_.record(b_.open,    b_.close,    k_var_x, val3_);
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(a_.open,    k_var_x), val2_));
    EXPECT_TRUE(same_value(bm_.query(b_.open,    k_var_x), val3_));
}

// ---------------------------------------------------------------------------
// Sibling isolation — the critical invariant.
// Catch bug: missing close event causes a sibling to inherit a binding it
// should not see.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, LeftSubtreeBindingInvisibleToRightSibling) {
    // Only A binds x; root never does. C should see nullopt.
    bm_.record(a_.open, a_.close, k_var_x, val2_);
    EXPECT_FALSE(bm_.query(c_.open, k_var_x).has_value());
}

TEST_F(FpBindMapTest, RightSubtreeBindingInvisibleToLeftSibling) {
    bm_.record(c_.open, c_.close, k_var_x, val5_);
    EXPECT_FALSE(bm_.query(a_.open, k_var_x).has_value());
}

TEST_F(FpBindMapTest, DeepLeftSubtreeInvisibleToRightSibling) {
    // B is deep inside A; C is A's sibling. C must not see B's binding.
    bm_.record(b_.open, b_.close, k_var_x, val3_);
    EXPECT_FALSE(bm_.query(c_.open, k_var_x).has_value());
}

TEST_F(FpBindMapTest, DeepRightSubtreeInvisibleToLeftSibling) {
    // D is deep inside C; A is C's sibling. A must not see D's binding.
    bm_.record(d_.open, d_.close, k_var_x, val4_);
    EXPECT_FALSE(bm_.query(a_.open, k_var_x).has_value());
}

TEST_F(FpBindMapTest, DeepLeftSubtreeInvisibleToDeepRightSubtree) {
    // B (in A's subtree) and D (in C's subtree) are in separate branches.
    bm_.record(b_.open, b_.close, k_var_x, val3_);
    EXPECT_FALSE(bm_.query(d_.open, k_var_x).has_value());
}

// ---------------------------------------------------------------------------
// Multiple variables
// Catch bug: timelines for different variables interfere with each other.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, TwoVarsBoundAtSameNodeBothVisible) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(root_.open, root_.close, k_var_y, val2_);
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_y), val2_));
}

TEST_F(FpBindMapTest, IndependentVarsDontCrossContaminate) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(a_.open,    a_.close,    k_var_y, val2_);
    // B (inside A) sees both.
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_y), val2_));
    // C (sibling of A) sees x but not y.
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
    EXPECT_FALSE(bm_.query(c_.open, k_var_y).has_value());
}

TEST_F(FpBindMapTest, RebindingOneVarLeavesOtherUntouched) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(root_.open, root_.close, k_var_y, val2_);
    bm_.record(a_.open,    a_.close,    k_var_x, val3_);
    // A sees rebinding of x, original y.
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_x), val3_));
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_y), val2_));
    // C sees original x (close event restores), original y.
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_y), val2_));
}

// ---------------------------------------------------------------------------
// Close-event correctness
// Catch bug: close event not recorded, or recorded with wrong prior value.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, CloseEventRestorationAfterSubtree) {
    // After A's interval, C should see root's value, not A's.
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(a_.open,    a_.close,    k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
}

TEST_F(FpBindMapTest, LinearChainEachLevelSeesOwnBinding) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(a_.open,    a_.close,    k_var_x, val2_);
    bm_.record(b_.open,    b_.close,    k_var_x, val3_);
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(a_.open,    k_var_x), val2_));
    EXPECT_TRUE(same_value(bm_.query(b_.open,    k_var_x), val3_));
    // C is a sibling of A — sees val1 restored by A's close event.
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
}

TEST_F(FpBindMapTest, NestedRebindRestoresCorrectlyAtOuterSibling) {
    // D is inside C; A is a sibling of C. A must not see C's or D's binding.
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(c_.open,    c_.close,    k_var_x, val2_);
    bm_.record(d_.open,    d_.close,    k_var_x, val3_);
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_x), val1_));
}

// ---------------------------------------------------------------------------
// Nullopt restoration
// Catch bug: close event records a non-nullopt value when the variable was
// previously unbound, causing the sibling to see a phantom binding.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, VarBoundOnlyInSiblingAppearsUnboundElsewhere) {
    // Only A binds x; root has no x. C should see nullopt.
    bm_.record(a_.open, a_.close, k_var_x, val2_);
    EXPECT_FALSE(bm_.query(c_.open, k_var_x).has_value());
}

TEST_F(FpBindMapTest, CloseEventExplicitlyRestoresNullopt) {
    // Root does not bind x; A does.  After A's close, C queries and should
    // get nullopt — the close event at A.close must restore nullopt.
    bm_.record(a_.open, a_.close, k_var_x, val2_);
    EXPECT_FALSE(bm_.query(c_.open, k_var_x).has_value());
}

TEST_F(FpBindMapTest, VarBoundInDeepSiblingSubtreeRestoresNulloptOutside) {
    // B (inside A) binds x; root and A don't. D (inside C) queries — nullopt.
    bm_.record(b_.open, b_.close, k_var_x, val3_);
    EXPECT_FALSE(bm_.query(d_.open, k_var_x).has_value());
}

// ---------------------------------------------------------------------------
// Query precision
// Catch bug: off-by-one in predecessor vs. successor, or using upper_bound
// instead of (upper_bound then --).
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, QueryAtExactOpenLabelReturnsOwnValue) {
    bm_.record(a_.open, a_.close, k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_x), val2_));
}

TEST_F(FpBindMapTest, QueryBeforeAOpenReturnsNullopt) {
    // A binds x; querying at root.open (before A's interval) → nullopt.
    bm_.record(a_.open, a_.close, k_var_x, val2_);
    EXPECT_FALSE(bm_.query(root_.open, k_var_x).has_value());
}

TEST_F(FpBindMapTest, QueryBeforeAOpenReturnsParentValue) {
    // Root binds x; A rebinds x.  Querying at root.open (before A's interval)
    // → root's value, not A's.
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(a_.open,    a_.close,    k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_x), val1_));
}

TEST_F(FpBindMapTest, QueryAtCloseEventPositionReturnsRestoredValue) {
    // The close event at A.close stores val1 (root's value).
    // Querying exactly at A.close should return val1.
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(a_.open,    a_.close,    k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(a_.close, k_var_x), val1_));
}

// ---------------------------------------------------------------------------
// Complex combined — omnibus test covering the full reference tree.
// A single bug in record or query will cause at least one assertion to fail.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, FullTreeInvariantCheck) {
    // Bindings:
    //   root: x=val1, y=val2
    //   A:    x=val3
    //   B:    y=val4
    //   C:    (nothing)
    //   D:    x=val5
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(root_.open, root_.close, k_var_y, val2_);
    bm_.record(a_.open,    a_.close,    k_var_x, val3_);
    bm_.record(b_.open,    b_.close,    k_var_y, val4_);
    bm_.record(d_.open,    d_.close,    k_var_x, val5_);

    // root: sees its own x and y
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_y), val2_));

    // A: x is shadowed by val3, y inherited from root
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_x), val3_));
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_y), val2_));

    // B (inside A): x = val3 (inherited from A), y = val4 (own binding)
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), val3_));
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_y), val4_));

    // C: x = val1 (A's close event restores root's value), y = val2
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_y), val2_));

    // D (inside C): x = val5 (own binding), y = val2 (inherited from root)
    EXPECT_TRUE(same_value(bm_.query(d_.open, k_var_x), val5_));
    EXPECT_TRUE(same_value(bm_.query(d_.open, k_var_y), val2_));

    // z: never bound anywhere → all nodes return nullopt
    EXPECT_FALSE(bm_.query(root_.open, k_var_z).has_value());
    EXPECT_FALSE(bm_.query(a_.open,    k_var_z).has_value());
    EXPECT_FALSE(bm_.query(b_.open,    k_var_z).has_value());
    EXPECT_FALSE(bm_.query(c_.open,    k_var_z).has_value());
    EXPECT_FALSE(bm_.query(d_.open,    k_var_z).has_value());
}

// ---------------------------------------------------------------------------
// Two siblings both binding the same variable — each isolated from the other.
// Catch bug: a sibling's binding leaks through because close events for two
// separate sibling records interact incorrectly on the same timeline.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, TwoSiblingsBindSameVarBothIsolated) {
    // A:x=val2, C:x=val5 — root never binds x.
    bm_.record(a_.open, a_.close, k_var_x, val2_);
    bm_.record(c_.open, c_.close, k_var_x, val5_);
    // B (inside A) should see A's value.
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), val2_));
    // D (inside C) should see C's value.
    EXPECT_TRUE(same_value(bm_.query(d_.open, k_var_x), val5_));
    // root is before both; no binding there.
    EXPECT_FALSE(bm_.query(root_.open, k_var_x).has_value());
    // A should not see C's binding, C should not see A's binding.
    EXPECT_FALSE(same_value(bm_.query(a_.open, k_var_x), val5_));
    EXPECT_FALSE(same_value(bm_.query(c_.open, k_var_x), val2_));
}

// ---------------------------------------------------------------------------
// Record only at a leaf; every ancestor and sibling returns nullopt.
// Catch bug: a record at a deep node somehow propagates upward in the timeline.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, RecordAtLeafOnlyAncestorsReturnNullopt) {
    // Only B is recorded; root, A, C, D are never recorded.
    bm_.record(b_.open, b_.close, k_var_x, val3_);
    EXPECT_FALSE(bm_.query(root_.open, k_var_x).has_value());
    EXPECT_FALSE(bm_.query(a_.open,    k_var_x).has_value());
    EXPECT_TRUE( same_value(bm_.query(b_.open,  k_var_x), val3_));
    EXPECT_FALSE(bm_.query(c_.open,    k_var_x).has_value());
    EXPECT_FALSE(bm_.query(d_.open,    k_var_x).has_value());
}

// ---------------------------------------------------------------------------
// frame_offset is carried through record and query without being lost or zeroed.
// Catch bug: implementation stores or compares only the skeleton pointer and
// ignores frame_offset.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, FrameOffsetPreservedInQuery) {
    const framed_expr with_offset = make_framed(99, 42);
    bm_.record(root_.open, root_.close, k_var_x, with_offset);
    const auto result = bm_.query(b_.open, k_var_x);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->frame_offset, 42u);
    EXPECT_EQ(result->skeleton, with_offset.skeleton);
}

TEST_F(FpBindMapTest, ZeroFrameOffsetDistinctFromNonZero) {
    const framed_expr offset_zero    = make_framed(10, 0);
    const framed_expr offset_nonzero = make_framed(10, 7);
    EXPECT_NE(offset_zero, offset_nonzero);
    bm_.record(root_.open, root_.close, k_var_x, offset_zero);
    bm_.record(a_.open,    a_.close,    k_var_x, offset_nonzero);
    // B (inside A) should see offset_nonzero.
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), offset_nonzero));
    // C (sibling of A) should see offset_zero (restored by A's close).
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), offset_zero));
}

// ---------------------------------------------------------------------------
// Many variables recorded at root — all must be independently visible at
// deep descendants.  Catch bug: the unordered_map for timelines has a
// collision or capacity issue that silently drops some variables.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, ManyVarsAtRootAllVisibleAtDeepDescendant) {
    constexpr int k_var_count = 20;
    std::vector<framed_expr> expected;
    expected.reserve(k_var_count);
    for (int var_idx = 0; var_idx < k_var_count; ++var_idx) {
        const framed_expr value = make_framed(static_cast<uint32_t>(100 + var_idx));
        expected.push_back(value);
        bm_.record(root_.open, root_.close,
                   static_cast<uint32_t>(1000 + var_idx), value);
    }
    for (int var_idx = 0; var_idx < k_var_count; ++var_idx) {
        const auto result = bm_.query(b_.open, static_cast<uint32_t>(1000 + var_idx));
        EXPECT_TRUE(same_value(result, expected[var_idx]))
            << "var " << var_idx << " not visible at deep descendant";
    }
}

// ---------------------------------------------------------------------------
// Query at a position between two sibling intervals (probe_ sits between
// A.close and C.open in the Euler-tour order).
// Catch bug: off-by-one in predecessor search returns A's open event instead
// of A's close event when querying positions between sibling intervals.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, QueryBetweenSiblingIntervalsMidpoint) {
    // probe_.open is strictly between A.close and C.open.
    // The predecessor in x's timeline is A's close event → val1.
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(a_.open,    a_.close,    k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(probe_.open, k_var_x), val1_));
}

// ---------------------------------------------------------------------------
// Double-record at the same node with the same variable — the second call
// must win (the first is overwritten).
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, DoubleRecordSameNodeSameVar) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(a_.open,    a_.close,    k_var_x, val2_);
    // Overwrite A's record with val3.
    bm_.record(a_.open, a_.close, k_var_x, val3_);
    // B (inside A) should see val3.
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), val3_));
    // C (sibling of A) should see root's value.
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
}

// ---------------------------------------------------------------------------
// Three-level close chain: root:x=1, A:x=2, B:x=3.
// Verifies that A's close correctly restores root's value (not B's),
// and that C (sibling of A) sees root's value and not A's or B's.
// Catch bug: close event computed from wrong predecessor (uses B's close's
// prior value instead of the actual ancestor's value for A's close event).
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, ThreeLevelCloseChainRestoresCorrectly) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    bm_.record(a_.open,    a_.close,    k_var_x, val2_);
    bm_.record(b_.open,    b_.close,    k_var_x, val3_);
    // A's close must restore val1 (root's value), not val2 (A's value).
    EXPECT_TRUE(same_value(bm_.query(a_.close, k_var_x), val1_));
    // B's close must restore val2 (A's value), not val1 (root's value).
    EXPECT_TRUE(same_value(bm_.query(b_.close, k_var_x), val2_));
    // C sees val1 restored by A's close.
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
}

// ---------------------------------------------------------------------------
// Query at root's open position returns root's own binding.
// Catch bug: predecessor search fails when the query position exactly equals
// the smallest key (lower_bound returns begin(), then -- underflows).
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, QueryAtRootOpenReturnsRootBinding) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_x), val1_));
}

// ---------------------------------------------------------------------------
// Query at root's close position (the close event restores nullopt) returns
// nullopt even though root bound x earlier.
// Catch bug: out-of-bounds in the timeline map causes a crash or wrong value.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, QueryAtRootCloseReturnsNullopt) {
    bm_.record(root_.open, root_.close, k_var_x, val1_);
    // The close event at root_.close stores nullopt (prior was unbound).
    EXPECT_FALSE(bm_.query(root_.close, k_var_x).has_value());
}

// ---------------------------------------------------------------------------
// Alternating-level bindings: bind at even depths (L0, L2, L4), not at odd.
// Odd depths must inherit from their nearest even ancestor.
// Catches bugs in the predecessor search when several levels lack a binding.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, AlternatingLevelBindingsOddLevelsInherit) {
    // 5-level linear chain using extra allocations from the fixture's om_.
    // probe_after_l2 sits inside L1's interval but after L2 — used to verify
    // that L0's value is restored once L2's interval ends.
    const om_interval l0             = om_.allocate_root();
    const om_interval l1             = om_.allocate_child_of(l0);
    const om_interval l2             = om_.allocate_child_of(l1);
    const om_interval l3             = om_.allocate_child_of(l2);
    const om_interval l4             = om_.allocate_child_of(l3);
    const om_interval probe_after_l2 = om_.allocate_child_of(l1);

    fp_bind_map bm;
    bm.record(l0.open, l0.close, k_var_x, val1_);  // L0: x = val1
    bm.record(l2.open, l2.close, k_var_x, val2_);  // L2: x = val2
    bm.record(l4.open, l4.close, k_var_x, val3_);  // L4: x = val3

    // L0: own binding.
    EXPECT_TRUE(same_value(bm.query(l0.open, k_var_x), val1_));
    // L1: odd, inherits from L0.
    EXPECT_TRUE(same_value(bm.query(l1.open, k_var_x), val1_));
    // L2: own binding.
    EXPECT_TRUE(same_value(bm.query(l2.open, k_var_x), val2_));
    // L3: odd, inherits from L2.
    EXPECT_TRUE(same_value(bm.query(l3.open, k_var_x), val2_));
    // L4: own binding.
    EXPECT_TRUE(same_value(bm.query(l4.open, k_var_x), val3_));
    // After L2's interval (probe_after_l2 is inside L1 but outside L2),
    // L0's value should be restored by L2's close event.
    EXPECT_TRUE(same_value(bm.query(probe_after_l2.open, k_var_x), val1_))
        << "after L2's interval, should see L0's value restored";
}

// ---------------------------------------------------------------------------
// Ten-level chain with five variables each bound at a different level.
// After all records, every (level, var) combination is verified.
// Catches bugs where close events at one variable's level interfere with
// another variable's timeline.
// ---------------------------------------------------------------------------

TEST_F(FpBindMapTest, TenLevelChainFiveVarsBoundAtDifferentLevels) {
    constexpr int k_levels = 10;
    constexpr int k_vars   = 5;

    // Allocate 10 nested levels using the fixture's om_.
    std::vector<om_interval> levels;
    levels.reserve(k_levels);
    levels.push_back(om_.allocate_root());
    for (int i = 1; i < k_levels; ++i)
        levels.push_back(om_.allocate_child_of(levels[i - 1]));

    const framed_expr level_vals[k_levels] = {
        val1_, val2_, val3_, val4_, val5_,
        make_framed(6), make_framed(7), make_framed(8), make_framed(9), make_framed(10)
    };

    fp_bind_map bm;
    // Bind var_idx at level (var_idx * 2): levels 0, 2, 4, 6, 8.
    for (int var_idx = 0; var_idx < k_vars; ++var_idx) {
        const int bound_level = var_idx * 2;
        bm.record(levels[bound_level].open, levels[bound_level].close,
                  static_cast<uint32_t>(var_idx), level_vals[bound_level]);
    }

    // For each (level, var) pair, verify the correct inherited value.
    for (int level = 0; level < k_levels; ++level) {
        for (int var_idx = 0; var_idx < k_vars; ++var_idx) {
            const int bound_level = var_idx * 2;
            if (level >= bound_level) {
                EXPECT_TRUE(same_value(bm.query(levels[level].open,
                                                static_cast<uint32_t>(var_idx)),
                                       level_vals[bound_level]))
                    << "level " << level << " var " << var_idx << " should see binding";
            } else {
                EXPECT_FALSE(bm.query(levels[level].open,
                                      static_cast<uint32_t>(var_idx)).has_value())
                    << "level " << level << " var " << var_idx << " should be unbound";
            }
        }
    }
}
