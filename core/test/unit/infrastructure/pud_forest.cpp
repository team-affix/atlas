// pud_forest: axiom forest with interned ids, ordered children, nested OM intervals.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <variant>
#include <vector>
#include "infrastructure/pud_forest.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_rule_id.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

struct MockMakeAxiom {
    MOCK_METHOD(const pud_rule_id*, make_axiom, (size_t), ());
};

struct MockMakeInference {
    MOCK_METHOD(const pud_rule_id*, make_inference,
                (const pud_rule_id*, size_t, const pud_rule_id*), ());
};

struct MockAllocateRootInterval {
    MOCK_METHOD(om_interval, allocate_root, (), ());
};

struct MockAllocateChildInterval {
    MOCK_METHOD(om_interval, allocate_child_of, (const om_interval&), ());
};

using test_forest_t = pud_forest<NiceMock<MockMakeAxiom>,
                                 NiceMock<MockMakeInference>,
                                 NiceMock<MockAllocateRootInterval>,
                                 NiceMock<MockAllocateChildInterval>>;

struct PudForestTest : public ::testing::Test {
    PudForestTest()
        : root_open_a_(10)
        , root_close_a_(40)
        , root_open_b_(50)
        , root_close_b_(80)
        , child_open_(15)
        , child_close_(20)
        , root_interval_a_{om_label(&root_open_a_), om_label(&root_close_a_)}
        , root_interval_b_{om_label(&root_open_b_), om_label(&root_close_b_)}
        , child_interval_{om_label(&child_open_), om_label(&child_close_)}
        , q_{expr::functor{1, {}}}
        , r_{expr::functor{2, {}}}
        , s_{expr::functor{3, {}}}
        , axiom0_{pud_rule_id::axiom{0}}
        , axiom1_{pud_rule_id::axiom{1}}
        , inference_{pud_rule_id::inference{&axiom0_, 0, &axiom0_}}
        , forest_(make_axiom_, make_inference_, allocate_root_, allocate_child_) {}

    uint64_t root_open_a_;
    uint64_t root_close_a_;
    uint64_t root_open_b_;
    uint64_t root_close_b_;
    uint64_t child_open_;
    uint64_t child_close_;
    om_interval root_interval_a_;
    om_interval root_interval_b_;
    om_interval child_interval_;
    expr q_;
    expr r_;
    expr s_;
    pud_rule_id axiom0_;
    pud_rule_id axiom1_;
    pud_rule_id inference_;
    NiceMock<MockMakeAxiom> make_axiom_;
    NiceMock<MockMakeInference> make_inference_;
    NiceMock<MockAllocateRootInterval> allocate_root_;
    NiceMock<MockAllocateChildInterval> allocate_child_;
    test_forest_t forest_;
};

TEST_F(PudForestTest, AddAxiomInternsStoresAsLeaf) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(allocate_root_, allocate_root()).WillOnce(Return(root_interval_a_));

    const pud_rule_id* id = forest_.add_axiom(0, {}, {&q_}, 1);
    EXPECT_EQ(id, &axiom0_);
    EXPECT_TRUE(forest_.is_leaf(id));
    EXPECT_EQ(forest_.try_parent(id), nullptr);
    EXPECT_EQ(forest_.get_node(id).lvc, 1u);
    EXPECT_EQ(forest_.get_node(id).added_body_goals.size(), 1u);
    EXPECT_EQ(forest_.get_node(id).interval.open.rank_ptr(), root_interval_a_.open.rank_ptr());
}

TEST_F(PudForestTest, TwoAxiomsAreIndependentLeaves) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(make_axiom_, make_axiom(1)).WillOnce(Return(&axiom1_));
    EXPECT_CALL(allocate_root_, allocate_root())
        .WillOnce(Return(root_interval_a_))
        .WillOnce(Return(root_interval_b_));

    const pud_rule_id* a0 = forest_.add_axiom(0, {}, {&q_}, 1);
    const pud_rule_id* a1 = forest_.add_axiom(1, {}, {&q_}, 1);
    EXPECT_TRUE(forest_.is_leaf(a0));
    EXPECT_TRUE(forest_.is_leaf(a1));
    EXPECT_EQ(forest_.try_parent(a0), nullptr);
    EXPECT_EQ(forest_.try_parent(a1), nullptr);
}

TEST_F(PudForestTest, LinkChildrenMakesParentNonLeafAndNestsChildInterval) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(make_inference_, make_inference(&axiom0_, 0, &axiom0_))
        .WillOnce(Return(&inference_));
    EXPECT_CALL(allocate_root_, allocate_root()).WillOnce(Return(root_interval_a_));
    EXPECT_CALL(allocate_child_, allocate_child_of(_))
        .WillOnce(Return(child_interval_));

    const pud_rule_id* parent = forest_.add_axiom(0, {}, {&q_}, 1);
    const pud_rule_id* child =
        forest_.add_inference(parent, 0, parent, {}, {&q_}, 1);
    EXPECT_FALSE(forest_.is_leaf(child));
    forest_.link_children(parent, {child});

    EXPECT_FALSE(forest_.is_leaf(parent));
    EXPECT_TRUE(forest_.is_leaf(child));
    EXPECT_EQ(forest_.try_parent(child), parent);
    EXPECT_EQ(forest_.ordered_children(parent),
              (std::vector<const pud_rule_id*>{child}));
    EXPECT_EQ(forest_.get_node(child).interval.open.rank_ptr(),
              child_interval_.open.rank_ptr());
}

TEST_F(PudForestTest, LinkChildrenOrdersByRuleIdNotPointer) {
    pud_rule_id inf_low{pud_rule_id::inference{&axiom0_, 0, &axiom1_}};
    pud_rule_id inf_high{pud_rule_id::inference{&axiom0_, 1, &axiom1_}};
    EXPECT_LT(inf_low, inf_high);

    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(make_inference_, make_inference(&axiom0_, 0, &axiom1_))
        .WillOnce(Return(&inf_low));
    EXPECT_CALL(make_inference_, make_inference(&axiom0_, 1, &axiom1_))
        .WillOnce(Return(&inf_high));
    EXPECT_CALL(allocate_root_, allocate_root()).WillOnce(Return(root_interval_a_));
    EXPECT_CALL(allocate_child_, allocate_child_of(_))
        .WillOnce(Return(child_interval_))
        .WillOnce(Return(child_interval_));

    const pud_rule_id* parent = forest_.add_axiom(0, {}, {&q_}, 1);
    const pud_rule_id* high =
        forest_.add_inference(parent, 1, &axiom1_, {}, {&q_}, 1);
    const pud_rule_id* low =
        forest_.add_inference(parent, 0, &axiom1_, {}, {&q_}, 1);
    forest_.link_children(parent, {high, low});

    const std::vector<const pud_rule_id*> ordered = forest_.ordered_children(parent);
    ASSERT_EQ(ordered.size(), 2u);
    EXPECT_EQ(ordered[0], low);
    EXPECT_EQ(ordered[1], high);
}

TEST_F(PudForestTest, SelfCalleeInferenceIsAllowedAsChild) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(make_inference_, make_inference(&axiom0_, 0, &axiom0_))
        .WillOnce(Return(&inference_));
    EXPECT_CALL(allocate_root_, allocate_root()).WillOnce(Return(root_interval_a_));
    EXPECT_CALL(allocate_child_, allocate_child_of(_))
        .WillOnce(Return(child_interval_));

    const pud_rule_id* parent = forest_.add_axiom(0, {}, {&q_}, 1);
    const pud_rule_id* self_child =
        forest_.add_inference(parent, 0, parent, {}, {&q_}, 1);
    forest_.link_children(parent, {self_child});
    EXPECT_EQ(std::get<pud_rule_id::inference>(self_child->content).callee, parent);
    EXPECT_EQ(forest_.ordered_children(parent),
              (std::vector<const pud_rule_id*>{self_child}));
}

TEST_F(PudForestTest, EffectiveBodyIsAxiomBodyAtARoot) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(allocate_root_, allocate_root()).WillOnce(Return(root_interval_a_));

    const pud_rule_id* axiom = forest_.add_axiom(0, {}, {&q_, &r_}, 1);
    EXPECT_EQ(forest_.effective_body(axiom), (std::vector<const expr*>{&q_, &r_}));
}

TEST_F(PudForestTest, EffectiveBodyDropsUnfoldedCallSiteAndAppendsAddedGoals) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(make_inference_, make_inference(&axiom0_, 0, &axiom0_))
        .WillOnce(Return(&inference_));
    EXPECT_CALL(allocate_root_, allocate_root()).WillOnce(Return(root_interval_a_));
    EXPECT_CALL(allocate_child_, allocate_child_of(_))
        .WillOnce(Return(child_interval_));

    const pud_rule_id* parent = forest_.add_axiom(0, {}, {&q_, &r_}, 1);
    const pud_rule_id* child = forest_.add_inference(parent, 0, parent, {}, {&s_}, 1);
    forest_.link_children(parent, {child});
    EXPECT_EQ(forest_.effective_body(child), (std::vector<const expr*>{&r_, &s_}));
}

TEST_F(PudForestTest, AddInferenceDoesNotAllocateARootInterval) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(make_inference_, make_inference(&axiom0_, 0, &axiom0_))
        .WillOnce(Return(&inference_));
    EXPECT_CALL(allocate_root_, allocate_root()).WillOnce(Return(root_interval_a_));
    EXPECT_CALL(allocate_child_, allocate_child_of(_)).Times(0);

    const pud_rule_id* parent = forest_.add_axiom(0, {}, {&q_}, 1);
    forest_.add_inference(parent, 0, parent, {}, {&q_}, 1);
}
