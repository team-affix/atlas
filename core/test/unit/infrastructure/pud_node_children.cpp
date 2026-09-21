// pud_node_children: store and get by id; absent get is a leaf.

#include <gtest/gtest.h>
#include <optional>
#include <set>
#include "infrastructure/pud_node_children.hpp"
#include "value_objects/pud_rule_id.hpp"

struct PudNodeChildrenTest : public ::testing::Test {
    PudNodeChildrenTest()
        : axiom0_{pud_rule_id::axiom{0}}
        , axiom1_{pud_rule_id::axiom{1}}
        , inference_{pud_rule_id::inference{&axiom0_, 0, &axiom0_}}
        , children_() {}

    pud_rule_id axiom0_;
    pud_rule_id axiom1_;
    pud_rule_id inference_;
    pud_node_children children_;
};

TEST_F(PudNodeChildrenTest, StoreThenGet) {
    children_.store(&axiom0_, {&inference_});
    const std::optional<std::set<const pud_rule_id*>> got = children_.get(&axiom0_);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(*got, (std::set<const pud_rule_id*>{&inference_}));
}

TEST_F(PudNodeChildrenTest, GetUnknownIsNullopt) {
    EXPECT_EQ(children_.get(&axiom0_), std::nullopt);
}

TEST_F(PudNodeChildrenTest, GetOrdersByPointer) {
    pud_rule_id inf_a{pud_rule_id::inference{&axiom0_, 0, &axiom1_}};
    pud_rule_id inf_b{pud_rule_id::inference{&axiom0_, 1, &axiom1_}};
    const pud_rule_id* low = &inf_a;
    const pud_rule_id* high = &inf_b;
    if (high < low) {
        low = &inf_b;
        high = &inf_a;
    }
    children_.store(&axiom0_, {high, low});
    const std::optional<std::set<const pud_rule_id*>> got = children_.get(&axiom0_);
    ASSERT_TRUE(got.has_value());
    ASSERT_EQ(got->size(), 2u);
    auto it = got->begin();
    EXPECT_EQ(*it, low);
    ++it;
    EXPECT_EQ(*it, high);
}
