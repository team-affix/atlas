// pud_forest: axiom forest with interned ids, ordered children, nested OM intervals.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <deque>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
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
    EXPECT_EQ(forest_.root_interval(id).open.rank_ptr(), root_interval_a_.open.rank_ptr());
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
    EXPECT_EQ(forest_.root_interval(child).open.rank_ptr(),
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

TEST_F(PudForestTest, AddInferenceDoesNotAllocateARootInterval) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(make_inference_, make_inference(&axiom0_, 0, &axiom0_))
        .WillOnce(Return(&inference_));
    EXPECT_CALL(allocate_root_, allocate_root()).WillOnce(Return(root_interval_a_));
    EXPECT_CALL(allocate_child_, allocate_child_of(_)).Times(0);

    const pud_rule_id* parent = forest_.add_axiom(0, {}, {&q_}, 1);
    const pud_rule_id* child =
        forest_.add_inference(parent, 0, parent, {}, {&q_}, 1);
    EXPECT_FALSE(forest_.is_leaf(child));
    EXPECT_EQ(forest_.try_parent(child), nullptr);
}

TEST_F(PudForestTest, OrderedChildrenUnknownParentIsEmpty) {
    EXPECT_TRUE(forest_.ordered_children(&axiom0_).empty());
}

TEST_F(PudForestTest, TryParentUnknownIsNull) {
    EXPECT_EQ(forest_.try_parent(&axiom0_), nullptr);
}

TEST_F(PudForestTest, IsLeafUnknownIsFalse) {
    EXPECT_FALSE(forest_.is_leaf(&axiom0_));
}

TEST_F(PudForestTest, GetNodeUnknownThrows) {
    EXPECT_THROW(forest_.get_node(&axiom0_), std::out_of_range);
}

TEST_F(PudForestTest, AddAxiomStoresAddedUnifications) {
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(allocate_root_, allocate_root()).WillOnce(Return(root_interval_a_));
    const pud_rule_id* id = forest_.add_axiom(0, {{0, &q_}}, {&r_}, 1);
    ASSERT_EQ(forest_.get_node(id).added_unifications.size(), 1u);
    EXPECT_EQ(forest_.get_node(id).added_unifications[0].var_idx, 0u);
    EXPECT_EQ(forest_.get_node(id).added_unifications[0].value, &q_);
}

TEST_F(PudForestTest, GrowOnlyLeafPartition) {
    EXPECT_CALL(make_axiom_, make_axiom(_)).WillRepeatedly(Return(&axiom0_));
    EXPECT_CALL(make_inference_, make_inference(_, _, _)).WillRepeatedly(Return(&inference_));
    EXPECT_CALL(allocate_root_, allocate_root()).WillRepeatedly(Return(root_interval_a_));
    EXPECT_CALL(allocate_child_, allocate_child_of(_)).WillRepeatedly(Return(child_interval_));

    const pud_rule_id* parent = forest_.add_axiom(0, {}, {&q_}, 1);
    EXPECT_TRUE(forest_.is_leaf(parent));
    EXPECT_TRUE(forest_.ordered_children(parent).empty());
    EXPECT_EQ(forest_.try_parent(parent), nullptr);

    const pud_rule_id* child = forest_.add_inference(parent, 0, parent, {}, {&q_}, 1);
    EXPECT_FALSE(forest_.is_leaf(child));
    EXPECT_EQ(forest_.try_parent(child), nullptr);
    EXPECT_TRUE(forest_.is_leaf(parent));

    forest_.link_children(parent, {child});
    EXPECT_FALSE(forest_.is_leaf(parent));
    EXPECT_TRUE(forest_.is_leaf(child));
    EXPECT_EQ(forest_.try_parent(child), parent);
    EXPECT_FALSE(forest_.ordered_children(parent).empty());
}

TEST_F(PudForestTest, StressManyChildrenOrdered) {
    std::vector<pud_rule_id> children;
    children.reserve(64);
    for (int idx = 0; idx < 64; ++idx)
        children.push_back(pud_rule_id{pud_rule_id::inference{
            &axiom0_, static_cast<size_t>(idx), &axiom1_}});
    EXPECT_CALL(make_axiom_, make_axiom(0)).WillOnce(Return(&axiom0_));
    EXPECT_CALL(allocate_root_, allocate_root()).WillOnce(Return(root_interval_a_));
    EXPECT_CALL(allocate_child_, allocate_child_of(_)).WillRepeatedly(Return(child_interval_));
    for (int idx = 0; idx < 64; ++idx) {
        EXPECT_CALL(make_inference_, make_inference(&axiom0_, static_cast<size_t>(idx), &axiom1_))
            .WillOnce(Return(&children[static_cast<size_t>(idx)]));
    }

    const pud_rule_id* parent = forest_.add_axiom(0, {}, {&q_}, 1);
    std::vector<const pud_rule_id*> linked;
    for (int idx = 63; idx >= 0; --idx) {
        const pud_rule_id* child = forest_.add_inference(
            parent, static_cast<size_t>(idx), &axiom1_, {}, {&q_}, 1);
        linked.push_back(child);
    }
    forest_.link_children(parent, linked);
    const std::vector<const pud_rule_id*> ordered = forest_.ordered_children(parent);
    ASSERT_EQ(ordered.size(), 64u);
    for (size_t idx = 1; idx < ordered.size(); ++idx)
        EXPECT_LT(*ordered[idx - 1], *ordered[idx]);
}

TEST_F(PudForestTest, FuzzAddAndLink) {
    std::deque<pud_rule_id> store;
    ON_CALL(make_axiom_, make_axiom(_)).WillByDefault([&store](size_t entry_idx) {
        store.push_back(pud_rule_id{pud_rule_id::axiom{entry_idx}});
        return &store.back();
    });
    ON_CALL(make_inference_, make_inference(_, _, _)).WillByDefault(
        [&store](const pud_rule_id* caller, size_t call_site, const pud_rule_id* callee) {
            store.push_back(pud_rule_id{pud_rule_id::inference{caller, call_site, callee}});
            return &store.back();
        });
    ON_CALL(allocate_root_, allocate_root()).WillByDefault(Return(root_interval_a_));
    ON_CALL(allocate_child_, allocate_child_of(_)).WillByDefault(Return(child_interval_));

    std::vector<const pud_rule_id*> axioms;
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
            const pud_rule_id* id = forest_.add_axiom(axioms.size(), {}, {&q_}, 1);
            axioms.push_back(id);
            leaves.insert(id);
            break;
        }
        case 1:
            if (!axioms.empty()) {
                const pud_rule_id* caller = axioms[rng() % axioms.size()];
                const pud_rule_id* id = forest_.add_inference(caller, 0, caller, {}, {&q_}, 1);
                unlinked.push_back(id);
                EXPECT_FALSE(forest_.is_leaf(id)) << "seed " << k_seed << " log " << log.str();
            }
            break;
        case 2: {
            std::vector<const pud_rule_id*> leaf_parents;
            for (const pud_rule_id* leaf : leaves)
                leaf_parents.push_back(leaf);
            if (leaf_parents.empty() || unlinked.empty())
                break;
            const pud_rule_id* parent = leaf_parents[rng() % leaf_parents.size()];
            const pud_rule_id* child = unlinked.back();
            unlinked.pop_back();
            forest_.link_children(parent, {child});
            leaves.erase(parent);
            leaves.insert(child);
            parents[child] = parent;
            break;
        }
        }
        for (const auto& [child, parent] : parents) {
            EXPECT_EQ(forest_.try_parent(child), parent)
                << "seed " << k_seed << " log " << log.str();
            EXPECT_FALSE(forest_.is_leaf(parent))
                << "seed " << k_seed << " log " << log.str();
        }
        for (const pud_rule_id* leaf : leaves)
            EXPECT_TRUE(forest_.is_leaf(leaf)) << "seed " << k_seed << " log " << log.str();
        for (const pud_rule_id* axiom : axioms)
            EXPECT_NO_THROW(forest_.get_node(axiom))
                << "seed " << k_seed << " log " << log.str();
        for (const auto& [child, parent] : parents)
            EXPECT_NO_THROW(forest_.get_node(child))
                << "seed " << k_seed << " log " << log.str();
    }
}
