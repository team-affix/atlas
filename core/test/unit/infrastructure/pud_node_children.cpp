// pud_node_children: add_root, link, ordered children, find_parent, is_leaf.

#include <gtest/gtest.h>
#include <deque>
#include <random>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
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

TEST_F(PudNodeChildrenTest, AddRootIsLeafWithNoParent) {
    children_.add_root(&axiom0_);
    EXPECT_TRUE(children_.is_leaf(&axiom0_));
    EXPECT_EQ(children_.find_parent(&axiom0_), nullptr);
    EXPECT_TRUE(children_.ordered_children(&axiom0_).empty());
}

TEST_F(PudNodeChildrenTest, TwoRootsAreIndependentLeaves) {
    children_.add_root(&axiom0_);
    children_.add_root(&axiom1_);
    EXPECT_TRUE(children_.is_leaf(&axiom0_));
    EXPECT_TRUE(children_.is_leaf(&axiom1_));
    EXPECT_EQ(children_.find_parent(&axiom0_), nullptr);
    EXPECT_EQ(children_.find_parent(&axiom1_), nullptr);
}

TEST_F(PudNodeChildrenTest, LinkChildrenMakesParentNonLeaf) {
    children_.add_root(&axiom0_);
    EXPECT_FALSE(children_.is_leaf(&inference_));
    children_.link_children(&axiom0_, {&inference_});
    EXPECT_FALSE(children_.is_leaf(&axiom0_));
    EXPECT_TRUE(children_.is_leaf(&inference_));
    EXPECT_EQ(children_.find_parent(&inference_), &axiom0_);
    EXPECT_EQ(children_.ordered_children(&axiom0_),
              (std::vector<const pud_rule_id*>{&inference_}));
}

TEST_F(PudNodeChildrenTest, LinkChildrenOrdersByRuleIdNotPointer) {
    pud_rule_id inf_low{pud_rule_id::inference{&axiom0_, 0, &axiom1_}};
    pud_rule_id inf_high{pud_rule_id::inference{&axiom0_, 1, &axiom1_}};
    EXPECT_LT(inf_low, inf_high);
    children_.add_root(&axiom0_);
    children_.link_children(&axiom0_, {&inf_high, &inf_low});
    const std::vector<const pud_rule_id*> ordered = children_.ordered_children(&axiom0_);
    ASSERT_EQ(ordered.size(), 2u);
    EXPECT_EQ(ordered[0], &inf_low);
    EXPECT_EQ(ordered[1], &inf_high);
}

TEST_F(PudNodeChildrenTest, UnlinkedIdIsNotALeaf) {
    children_.add_root(&axiom0_);
    EXPECT_FALSE(children_.is_leaf(&inference_));
    EXPECT_EQ(children_.find_parent(&inference_), nullptr);
}

TEST_F(PudNodeChildrenTest, OrderedChildrenUnknownParentIsEmpty) {
    EXPECT_TRUE(children_.ordered_children(&axiom0_).empty());
}

TEST_F(PudNodeChildrenTest, FindParentUnknownIsNull) {
    EXPECT_EQ(children_.find_parent(&axiom0_), nullptr);
}

TEST_F(PudNodeChildrenTest, IsLeafUnknownIsFalse) {
    EXPECT_FALSE(children_.is_leaf(&axiom0_));
}

TEST_F(PudNodeChildrenTest, StressManyChildrenOrdered) {
    std::vector<pud_rule_id> kids;
    kids.reserve(64);
    for (int idx = 0; idx < 64; ++idx)
        kids.push_back(pud_rule_id{pud_rule_id::inference{
            &axiom0_, static_cast<size_t>(idx), &axiom1_}});
    children_.add_root(&axiom0_);
    std::vector<const pud_rule_id*> linked;
    for (int idx = 63; idx >= 0; --idx)
        linked.push_back(&kids[static_cast<size_t>(idx)]);
    children_.link_children(&axiom0_, linked);
    const std::vector<const pud_rule_id*> ordered = children_.ordered_children(&axiom0_);
    ASSERT_EQ(ordered.size(), 64u);
    for (size_t idx = 1; idx < ordered.size(); ++idx)
        EXPECT_LT(*ordered[idx - 1], *ordered[idx]);
}

TEST_F(PudNodeChildrenTest, FuzzAddRootAndLink) {
    std::deque<pud_rule_id> store;
    std::vector<const pud_rule_id*> roots;
    std::vector<const pud_rule_id*> unlinked;
    std::unordered_set<const pud_rule_id*> leaves;
    std::unordered_map<const pud_rule_id*, const pud_rule_id*> parents;
    constexpr uint32_t k_seed = 42;
    std::mt19937 rng{k_seed};
    std::uniform_int_distribution<int> op_dist(0, 2);
    std::ostringstream log;
    for (int step = 0; step < 80; ++step) {
        const int op = op_dist(rng);
        log << step << ':' << op << ' ';
        switch (op) {
        case 0: {
            store.push_back(pud_rule_id{pud_rule_id::axiom{store.size()}});
            const pud_rule_id* id = &store.back();
            children_.add_root(id);
            roots.push_back(id);
            leaves.insert(id);
            break;
        }
        case 1:
            if (!roots.empty()) {
                const pud_rule_id* caller = roots[rng() % roots.size()];
                store.push_back(pud_rule_id{pud_rule_id::inference{caller, 0, caller}});
                unlinked.push_back(&store.back());
                EXPECT_FALSE(children_.is_leaf(&store.back()))
                    << "seed " << k_seed << " log " << log.str();
            }
            break;
        case 2: {
            std::vector<const pud_rule_id*> leaf_parents(leaves.begin(), leaves.end());
            if (leaf_parents.empty() || unlinked.empty())
                break;
            const pud_rule_id* parent = leaf_parents[rng() % leaf_parents.size()];
            const pud_rule_id* child = unlinked.back();
            unlinked.pop_back();
            children_.link_children(parent, {child});
            leaves.erase(parent);
            leaves.insert(child);
            parents[child] = parent;
            break;
        }
        }
        for (const auto& [child, parent] : parents) {
            EXPECT_EQ(children_.find_parent(child), parent)
                << "seed " << k_seed << " log " << log.str();
            EXPECT_FALSE(children_.is_leaf(parent))
                << "seed " << k_seed << " log " << log.str();
        }
        for (const pud_rule_id* leaf : leaves)
            EXPECT_TRUE(children_.is_leaf(leaf)) << "seed " << k_seed << " log " << log.str();
    }
}
