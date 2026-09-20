// pud: axiom forest with interned ids, ordered children, nested OM intervals.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/pud.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_db_node.hpp"
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

using test_pud_t = pud<NiceMock<MockMakeAxiom>,
                       NiceMock<MockMakeInference>,
                       NiceMock<MockAllocateRootInterval>,
                       NiceMock<MockAllocateChildInterval>>;

struct PudTest : public ::testing::Test {
    PudTest()
        : root_open_a_(10)
        , root_close_a_(40)
        , root_open_b_(50)
        , root_close_b_(80)
        , child_open_(15)
        , child_close_(20)
        , root_interval_a_{om_label(&root_open_a_), om_label(&root_close_a_)}
        , root_interval_b_{om_label(&root_open_b_), om_label(&root_close_b_)}
        , child_interval_{om_label(&child_open_), om_label(&child_close_)}
        , body_{expr::var{0}}
        , axiom0_{pud_rule_id::axiom{0}}
        , axiom1_{pud_rule_id::axiom{1}}
        , inference_{pud_rule_id::inference{&axiom0_, 0, &axiom0_}}
        , forest_(make_axiom_, make_inference_, allocate_root_, allocate_child_) {}

    pud_db_node make_payload() {
        return pud_db_node{root_interval_a_, {}, {&body_}, 1};
    }

    uint64_t root_open_a_;
    uint64_t root_close_a_;
    uint64_t root_open_b_;
    uint64_t root_close_b_;
    uint64_t child_open_;
    uint64_t child_close_;
    om_interval root_interval_a_;
    om_interval root_interval_b_;
    om_interval child_interval_;
    expr body_;
    pud_rule_id axiom0_;
    pud_rule_id axiom1_;
    pud_rule_id inference_;
    NiceMock<MockMakeAxiom> make_axiom_;
    NiceMock<MockMakeInference> make_inference_;
    NiceMock<MockAllocateRootInterval> allocate_root_;
    NiceMock<MockAllocateChildInterval> allocate_child_;
    test_pud_t forest_;
};

TEST_F(PudTest, AddAxiomInternsStoresAsRootAndLeaf) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(allocate_root_, allocate_root()).WillOnce(Return(root_interval_a_));

    const pud_rule_id* id = forest_.add_axiom(0, make_payload());
    EXPECT_EQ(id, &axiom0_);
    EXPECT_TRUE(forest_.roots().contains(id));
    EXPECT_TRUE(forest_.leaves().contains(id));
    EXPECT_TRUE(forest_.is_leaf(id));
    EXPECT_EQ(forest_.get_node(id).lvc, 1u);
    EXPECT_EQ(forest_.get_node(id).added_body_goals.size(), 1u);
    EXPECT_EQ(forest_.get_node(id).interval.open.rank_ptr(), root_interval_a_.open.rank_ptr());
}

TEST_F(PudTest, TwoAxiomsAreIndependentRoots) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(make_axiom_, make_axiom(1)).WillOnce(Return(&axiom1_));
    EXPECT_CALL(allocate_root_, allocate_root())
        .WillOnce(Return(root_interval_a_))
        .WillOnce(Return(root_interval_b_));

    const pud_rule_id* a0 = forest_.add_axiom(0, make_payload());
    const pud_rule_id* a1 = forest_.add_axiom(1, make_payload());
    EXPECT_TRUE(forest_.roots().contains(a0));
    EXPECT_TRUE(forest_.roots().contains(a1));
    EXPECT_EQ(forest_.roots().size(), 2u);
    EXPECT_EQ(forest_.leaves().size(), 2u);
}

TEST_F(PudTest, LinkMakesParentNonLeafAndNestsChildInterval) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(make_inference_, make_inference(&axiom0_, 0, &axiom0_))
        .WillOnce(Return(&inference_));
    EXPECT_CALL(allocate_root_, allocate_root())
        .WillOnce(Return(root_interval_a_))
        .WillOnce(Return(root_interval_b_));
    EXPECT_CALL(allocate_child_, allocate_child_of(_))
        .WillOnce(Return(child_interval_));

    const pud_rule_id* parent = forest_.add_axiom(0, make_payload());
    const pud_rule_id* child =
        forest_.add_inference(parent, 0, parent, make_payload());
    forest_.link(parent, test_pud_t::child_set_t{child});

    EXPECT_FALSE(forest_.is_leaf(parent));
    EXPECT_TRUE(forest_.is_leaf(child));
    EXPECT_FALSE(forest_.roots().contains(child));
    EXPECT_TRUE(forest_.roots().contains(parent));
    EXPECT_EQ(forest_.parent(child), parent);
    EXPECT_TRUE(forest_.children(parent).contains(child));
    EXPECT_EQ(forest_.get_node(child).interval.open.rank_ptr(),
              child_interval_.open.rank_ptr());
}

TEST_F(PudTest, ChildrenAreOrderedByRuleIdNotPointer) {
    pud_rule_id inf_low{pud_rule_id::inference{&axiom0_, 0, &axiom1_}};
    pud_rule_id inf_high{pud_rule_id::inference{&axiom0_, 1, &axiom1_}};
    EXPECT_LT(inf_low, inf_high);

    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(make_inference_, make_inference(&axiom0_, 0, &axiom1_))
        .WillOnce(Return(&inf_low));
    EXPECT_CALL(make_inference_, make_inference(&axiom0_, 1, &axiom1_))
        .WillOnce(Return(&inf_high));
    EXPECT_CALL(allocate_root_, allocate_root())
        .WillOnce(Return(root_interval_a_))
        .WillOnce(Return(root_interval_b_))
        .WillOnce(Return(root_interval_b_));
    EXPECT_CALL(allocate_child_, allocate_child_of(_))
        .WillOnce(Return(child_interval_))
        .WillOnce(Return(child_interval_));

    const pud_rule_id* parent = forest_.add_axiom(0, make_payload());
    const pud_rule_id* high =
        forest_.add_inference(parent, 1, &axiom1_, make_payload());
    const pud_rule_id* low =
        forest_.add_inference(parent, 0, &axiom1_, make_payload());
    forest_.link(parent, test_pud_t::child_set_t{high, low});

    auto it = forest_.children(parent).begin();
    EXPECT_EQ(*it, low);
    ++it;
    EXPECT_EQ(*it, high);
}

TEST_F(PudTest, LinkChildrenMatchesLinkOrderByRuleId) {
    pud_rule_id inf_low{pud_rule_id::inference{&axiom0_, 0, &axiom1_}};
    pud_rule_id inf_high{pud_rule_id::inference{&axiom0_, 1, &axiom1_}};
    EXPECT_LT(inf_low, inf_high);

    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(make_inference_, make_inference(&axiom0_, 0, &axiom1_))
        .WillOnce(Return(&inf_low));
    EXPECT_CALL(make_inference_, make_inference(&axiom0_, 1, &axiom1_))
        .WillOnce(Return(&inf_high));
    EXPECT_CALL(allocate_root_, allocate_root())
        .WillOnce(Return(root_interval_a_))
        .WillOnce(Return(root_interval_b_))
        .WillOnce(Return(root_interval_b_));
    EXPECT_CALL(allocate_child_, allocate_child_of(_))
        .WillOnce(Return(child_interval_))
        .WillOnce(Return(child_interval_));

    const pud_rule_id* parent = forest_.add_axiom(0, make_payload());
    const pud_rule_id* high =
        forest_.add_inference(parent, 1, &axiom1_, make_payload());
    const pud_rule_id* low =
        forest_.add_inference(parent, 0, &axiom1_, make_payload());
    forest_.link_children(parent, std::vector<const pud_rule_id*>{high, low});

    const std::vector<const pud_rule_id*> ordered = forest_.ordered_children(parent);
    ASSERT_EQ(ordered.size(), 2u);
    EXPECT_EQ(ordered[0], low);
    EXPECT_EQ(ordered[1], high);
}

TEST_F(PudTest, UnlinkRestoresChildAsRootAndParentAsLeafWhenLastChild) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(make_inference_, make_inference(&axiom0_, 0, &axiom0_))
        .WillOnce(Return(&inference_));
    EXPECT_CALL(allocate_root_, allocate_root())
        .WillOnce(Return(root_interval_a_))
        .WillOnce(Return(root_interval_b_));
    EXPECT_CALL(allocate_child_, allocate_child_of(_))
        .WillOnce(Return(child_interval_));

    const pud_rule_id* parent = forest_.add_axiom(0, make_payload());
    const pud_rule_id* child =
        forest_.add_inference(parent, 0, parent, make_payload());
    forest_.link(parent, test_pud_t::child_set_t{child});
    forest_.unlink(child);

    EXPECT_TRUE(forest_.is_leaf(parent));
    EXPECT_TRUE(forest_.roots().contains(child));
    EXPECT_TRUE(forest_.is_leaf(child));
}

TEST_F(PudTest, EraseRemovesIsolatedRootLeaf) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(allocate_root_, allocate_root()).WillOnce(Return(root_interval_a_));

    const pud_rule_id* id = forest_.add_axiom(0, make_payload());
    forest_.erase(id);
    EXPECT_TRUE(forest_.roots().empty());
    EXPECT_TRUE(forest_.leaves().empty());
}

TEST_F(PudTest, SelfCalleeInferenceIsAllowedAsChild) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(make_inference_, make_inference(&axiom0_, 0, &axiom0_))
        .WillOnce(Return(&inference_));
    EXPECT_CALL(allocate_root_, allocate_root())
        .WillOnce(Return(root_interval_a_))
        .WillOnce(Return(root_interval_b_));
    EXPECT_CALL(allocate_child_, allocate_child_of(_))
        .WillOnce(Return(child_interval_));

    const pud_rule_id* parent = forest_.add_axiom(0, make_payload());
    const pud_rule_id* self_child =
        forest_.add_inference(parent, 0, parent, make_payload());
    forest_.link(parent, test_pud_t::child_set_t{self_child});
    EXPECT_EQ(std::get<pud_rule_id::inference>(self_child->content).callee, parent);
    EXPECT_TRUE(forest_.children(parent).contains(self_child));
}
