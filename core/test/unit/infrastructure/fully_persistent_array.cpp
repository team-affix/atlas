#include <gtest/gtest.h>
#include <deque>
#include <vector>
#include "infrastructure/fully_persistent_array.hpp"
#include "infrastructure/order_maintenance.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

namespace {

framed_expr make_framed(uint32_t functor_id, uint32_t frame_offset = 0) {
    static std::deque<expr> exprs;
    exprs.push_back(expr{expr::functor{functor_id, {}}});
    return framed_expr{&exprs.back(), frame_offset};
}

bool same_value(const std::optional<framed_expr>& result, const framed_expr& expected) {
    return result.has_value() && *result == expected;
}

}

struct FullyPersistentArrayTest : public ::testing::Test {
    FullyPersistentArrayTest()
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
    fully_persistent_array bm_;
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

TEST_F(FullyPersistentArrayTest, EmptyMapReturnsNullopt) {
    EXPECT_FALSE(bm_.query(root_.open, k_var_x).has_value());
}

TEST_F(FullyPersistentArrayTest, UnrecordedVarReturnsNullopt) {
    bm_.record(root_, k_var_x, val1_);
    EXPECT_FALSE(bm_.query(root_.open, k_var_y).has_value());
}

TEST_F(FullyPersistentArrayTest, QueryAtRecordingNodeReturnsValue) {
    bm_.record(root_, k_var_x, val1_);
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, ChildInheritsAncestorBinding) {
    bm_.record(root_, k_var_x, val1_);
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, GrandchildInheritsGrandparentBinding) {
    bm_.record(root_, k_var_x, val1_);
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, StarTreeAllDescendantsInheritRoot) {
    bm_.record(root_, k_var_x, val1_);
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(d_.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, ChildRebindingHidesParent) {
    bm_.record(root_, k_var_x, val1_);
    bm_.record(a_, k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_x), val2_));
}

TEST_F(FullyPersistentArrayTest, ParentUnaffectedByChildRebinding) {
    bm_.record(root_, k_var_x, val1_);
    bm_.record(a_, k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, SiblingAfterRebindingChildSeesParent) {

    bm_.record(root_, k_var_x, val1_);
    bm_.record(a_, k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, GrandchildRebindingChain) {
    bm_.record(root_, k_var_x, val1_);
    bm_.record(a_, k_var_x, val2_);
    bm_.record(b_, k_var_x, val3_);
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(a_.open,    k_var_x), val2_));
    EXPECT_TRUE(same_value(bm_.query(b_.open,    k_var_x), val3_));
}

TEST_F(FullyPersistentArrayTest, LeftSubtreeBindingInvisibleToRightSibling) {

    bm_.record(a_, k_var_x, val2_);
    EXPECT_FALSE(bm_.query(c_.open, k_var_x).has_value());
}

TEST_F(FullyPersistentArrayTest, RightSubtreeBindingInvisibleToLeftSibling) {
    bm_.record(c_, k_var_x, val5_);
    EXPECT_FALSE(bm_.query(a_.open, k_var_x).has_value());
}

TEST_F(FullyPersistentArrayTest, DeepLeftSubtreeInvisibleToRightSibling) {

    bm_.record(b_, k_var_x, val3_);
    EXPECT_FALSE(bm_.query(c_.open, k_var_x).has_value());
}

TEST_F(FullyPersistentArrayTest, DeepRightSubtreeInvisibleToLeftSibling) {

    bm_.record(d_, k_var_x, val4_);
    EXPECT_FALSE(bm_.query(a_.open, k_var_x).has_value());
}

TEST_F(FullyPersistentArrayTest, DeepLeftSubtreeInvisibleToDeepRightSubtree) {

    bm_.record(b_, k_var_x, val3_);
    EXPECT_FALSE(bm_.query(d_.open, k_var_x).has_value());
}

TEST_F(FullyPersistentArrayTest, TwoVarsBoundAtSameNodeBothVisible) {
    bm_.record(root_, k_var_x, val1_);
    bm_.record(root_, k_var_y, val2_);
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_y), val2_));
}

TEST_F(FullyPersistentArrayTest, IndependentVarsDontCrossContaminate) {
    bm_.record(root_, k_var_x, val1_);
    bm_.record(a_, k_var_y, val2_);

    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_y), val2_));

    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
    EXPECT_FALSE(bm_.query(c_.open, k_var_y).has_value());
}

TEST_F(FullyPersistentArrayTest, RebindingOneVarLeavesOtherUntouched) {
    bm_.record(root_, k_var_x, val1_);
    bm_.record(root_, k_var_y, val2_);
    bm_.record(a_, k_var_x, val3_);

    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_x), val3_));
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_y), val2_));

    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_y), val2_));
}

TEST_F(FullyPersistentArrayTest, CloseEventRestorationAfterSubtree) {

    bm_.record(root_, k_var_x, val1_);
    bm_.record(a_, k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, LinearChainEachLevelSeesOwnBinding) {
    bm_.record(root_, k_var_x, val1_);
    bm_.record(a_, k_var_x, val2_);
    bm_.record(b_, k_var_x, val3_);
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(a_.open,    k_var_x), val2_));
    EXPECT_TRUE(same_value(bm_.query(b_.open,    k_var_x), val3_));

    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, NestedRebindRestoresCorrectlyAtOuterSibling) {

    bm_.record(root_, k_var_x, val1_);
    bm_.record(c_, k_var_x, val2_);
    bm_.record(d_, k_var_x, val3_);
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, VarBoundOnlyInSiblingAppearsUnboundElsewhere) {

    bm_.record(a_, k_var_x, val2_);
    EXPECT_FALSE(bm_.query(c_.open, k_var_x).has_value());
}

TEST_F(FullyPersistentArrayTest, CloseEventExplicitlyRestoresNullopt) {

    bm_.record(a_, k_var_x, val2_);
    EXPECT_FALSE(bm_.query(c_.open, k_var_x).has_value());
}

TEST_F(FullyPersistentArrayTest, VarBoundInDeepSiblingSubtreeRestoresNulloptOutside) {

    bm_.record(b_, k_var_x, val3_);
    EXPECT_FALSE(bm_.query(d_.open, k_var_x).has_value());
}

TEST_F(FullyPersistentArrayTest, QueryAtExactOpenLabelReturnsOwnValue) {
    bm_.record(a_, k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_x), val2_));
}

TEST_F(FullyPersistentArrayTest, QueryBeforeAOpenReturnsNullopt) {

    bm_.record(a_, k_var_x, val2_);
    EXPECT_FALSE(bm_.query(root_.open, k_var_x).has_value());
}

TEST_F(FullyPersistentArrayTest, QueryBeforeAOpenReturnsParentValue) {

    bm_.record(root_, k_var_x, val1_);
    bm_.record(a_, k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, QueryAtCloseEventPositionReturnsRestoredValue) {

    bm_.record(root_, k_var_x, val1_);
    bm_.record(a_, k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(a_.close, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, FullTreeInvariantCheck) {

    bm_.record(root_, k_var_x, val1_);
    bm_.record(root_, k_var_y, val2_);
    bm_.record(a_, k_var_x, val3_);
    bm_.record(b_, k_var_y, val4_);
    bm_.record(d_, k_var_x, val5_);

    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_y), val2_));

    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_x), val3_));
    EXPECT_TRUE(same_value(bm_.query(a_.open, k_var_y), val2_));

    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), val3_));
    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_y), val4_));

    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_y), val2_));

    EXPECT_TRUE(same_value(bm_.query(d_.open, k_var_x), val5_));
    EXPECT_TRUE(same_value(bm_.query(d_.open, k_var_y), val2_));

    EXPECT_FALSE(bm_.query(root_.open, k_var_z).has_value());
    EXPECT_FALSE(bm_.query(a_.open,    k_var_z).has_value());
    EXPECT_FALSE(bm_.query(b_.open,    k_var_z).has_value());
    EXPECT_FALSE(bm_.query(c_.open,    k_var_z).has_value());
    EXPECT_FALSE(bm_.query(d_.open,    k_var_z).has_value());
}

TEST_F(FullyPersistentArrayTest, TwoSiblingsBindSameVarBothIsolated) {

    bm_.record(a_, k_var_x, val2_);
    bm_.record(c_, k_var_x, val5_);

    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), val2_));

    EXPECT_TRUE(same_value(bm_.query(d_.open, k_var_x), val5_));

    EXPECT_FALSE(bm_.query(root_.open, k_var_x).has_value());

    EXPECT_FALSE(same_value(bm_.query(a_.open, k_var_x), val5_));
    EXPECT_FALSE(same_value(bm_.query(c_.open, k_var_x), val2_));
}

TEST_F(FullyPersistentArrayTest, RecordAtLeafOnlyAncestorsReturnNullopt) {

    bm_.record(b_, k_var_x, val3_);
    EXPECT_FALSE(bm_.query(root_.open, k_var_x).has_value());
    EXPECT_FALSE(bm_.query(a_.open,    k_var_x).has_value());
    EXPECT_TRUE( same_value(bm_.query(b_.open,  k_var_x), val3_));
    EXPECT_FALSE(bm_.query(c_.open,    k_var_x).has_value());
    EXPECT_FALSE(bm_.query(d_.open,    k_var_x).has_value());
}

TEST_F(FullyPersistentArrayTest, FrameOffsetPreservedInQuery) {
    const framed_expr with_offset = make_framed(99, 42);
    bm_.record(root_, k_var_x, with_offset);
    const auto result = bm_.query(b_.open, k_var_x);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->frame_offset, 42u);
    EXPECT_EQ(result->skeleton, with_offset.skeleton);
}

TEST_F(FullyPersistentArrayTest, ZeroFrameOffsetDistinctFromNonZero) {
    const framed_expr offset_zero    = make_framed(10, 0);
    const framed_expr offset_nonzero = make_framed(10, 7);
    EXPECT_NE(offset_zero, offset_nonzero);
    bm_.record(root_, k_var_x, offset_zero);
    bm_.record(a_, k_var_x, offset_nonzero);

    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), offset_nonzero));

    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), offset_zero));
}

TEST_F(FullyPersistentArrayTest, ManyVarsAtRootAllVisibleAtDeepDescendant) {
    constexpr int k_var_count = 20;
    std::vector<framed_expr> expected;
    expected.reserve(k_var_count);
    for (int var_idx = 0; var_idx < k_var_count; ++var_idx) {
        const framed_expr value = make_framed(static_cast<uint32_t>(100 + var_idx));
        expected.push_back(value);
        bm_.record(root_,
                   static_cast<uint32_t>(1000 + var_idx), value);
    }
    for (int var_idx = 0; var_idx < k_var_count; ++var_idx) {
        const auto result = bm_.query(b_.open, static_cast<uint32_t>(1000 + var_idx));
        EXPECT_TRUE(same_value(result, expected[var_idx]))
            << "var " << var_idx << " not visible at deep descendant";
    }
}

TEST_F(FullyPersistentArrayTest, QueryBetweenSiblingIntervalsMidpoint) {

    bm_.record(root_, k_var_x, val1_);
    bm_.record(a_, k_var_x, val2_);
    EXPECT_TRUE(same_value(bm_.query(probe_.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, DoubleRecordSameNodeSameVar) {
    bm_.record(root_, k_var_x, val1_);
    bm_.record(a_, k_var_x, val2_);

    bm_.record(a_, k_var_x, val3_);

    EXPECT_TRUE(same_value(bm_.query(b_.open, k_var_x), val3_));

    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, ThreeLevelCloseChainRestoresCorrectly) {
    bm_.record(root_, k_var_x, val1_);
    bm_.record(a_, k_var_x, val2_);
    bm_.record(b_, k_var_x, val3_);

    EXPECT_TRUE(same_value(bm_.query(a_.close, k_var_x), val1_));

    EXPECT_TRUE(same_value(bm_.query(b_.close, k_var_x), val2_));

    EXPECT_TRUE(same_value(bm_.query(c_.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, QueryAtRootOpenReturnsRootBinding) {
    bm_.record(root_, k_var_x, val1_);
    EXPECT_TRUE(same_value(bm_.query(root_.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayTest, QueryAtRootCloseReturnsNullopt) {
    bm_.record(root_, k_var_x, val1_);

    EXPECT_FALSE(bm_.query(root_.close, k_var_x).has_value());
}

TEST_F(FullyPersistentArrayTest, AlternatingLevelBindingsOddLevelsInherit) {

    const om_interval l0             = om_.allocate_root();
    const om_interval l1             = om_.allocate_child_of(l0);
    const om_interval l2             = om_.allocate_child_of(l1);
    const om_interval l3             = om_.allocate_child_of(l2);
    const om_interval l4             = om_.allocate_child_of(l3);
    const om_interval probe_after_l2 = om_.allocate_child_of(l1);

    fully_persistent_array bm;
    bm.record(l0, k_var_x, val1_);
    bm.record(l2, k_var_x, val2_);
    bm.record(l4, k_var_x, val3_);

    EXPECT_TRUE(same_value(bm.query(l0.open, k_var_x), val1_));

    EXPECT_TRUE(same_value(bm.query(l1.open, k_var_x), val1_));

    EXPECT_TRUE(same_value(bm.query(l2.open, k_var_x), val2_));

    EXPECT_TRUE(same_value(bm.query(l3.open, k_var_x), val2_));

    EXPECT_TRUE(same_value(bm.query(l4.open, k_var_x), val3_));

    EXPECT_TRUE(same_value(bm.query(probe_after_l2.open, k_var_x), val1_))
        << "after L2's interval, should see L0's value restored";
}

TEST_F(FullyPersistentArrayTest, TenLevelChainFiveVarsBoundAtDifferentLevels) {
    constexpr int k_levels = 10;
    constexpr int k_vars   = 5;

    std::vector<om_interval> levels;
    levels.reserve(k_levels);
    levels.push_back(om_.allocate_root());
    for (int i = 1; i < k_levels; ++i)
        levels.push_back(om_.allocate_child_of(levels[i - 1]));

    const framed_expr level_vals[k_levels] = {
        val1_, val2_, val3_, val4_, val5_,
        make_framed(6), make_framed(7), make_framed(8), make_framed(9), make_framed(10)
    };

    fully_persistent_array bm;

    for (int var_idx = 0; var_idx < k_vars; ++var_idx) {
        const int bound_level = var_idx * 2;
        bm.record(levels[bound_level],
                  static_cast<uint32_t>(var_idx), level_vals[bound_level]);
    }

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
