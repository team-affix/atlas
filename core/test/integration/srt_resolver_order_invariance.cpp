// Integration: srt_subgoals_activator → subgoals_activator → srt_active_goals / series_reduced_tree.
// Invariant: for the same unordered multiset of resolutions, valid permutations of resolve order
// leave the entire SRT forest identical (checked via locator-bound i_iterate_root_goals /
// i_iterate_child_goals). resolver steps 2–3 (candidate/goal deactivators) do not touch the tree.

#include <algorithm>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "infrastructure/coroutine.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/locator.hpp"
#include "infrastructure/resolver.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "infrastructure/srt_subgoals_activator.hpp"
#include "infrastructure/subgoals_activator.hpp"
#include "interfaces/i_activate_goal_candidates.hpp"
#include "interfaces/i_activate_subgoals_and_candidates.hpp"
#include "interfaces/i_active_goals_size.hpp"
#include "interfaces/i_check_active_goals_empty.hpp"
#include "interfaces/i_clear_active_goals.hpp"
#include "interfaces/i_deactivate_goal_candidates.hpp"
#include "interfaces/i_get_rule.hpp"
#include "interfaces/i_goal_activator.hpp"
#include "interfaces/i_goal_deactivator.hpp"
#include "interfaces/i_insert_active_goal.hpp"
#include "interfaces/i_is_active_goal.hpp"
#include "interfaces/i_iterate_child_goals.hpp"
#include "interfaces/i_iterate_root_goals.hpp"
#include "interfaces/i_make_goal_lineage.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_srt_flush_goal_batch.hpp"
#include "interfaces/i_srt_link_goal_batch_parent.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

using ::testing::Return;

namespace {

static constexpr rule_id kGroundRule = 0;
static constexpr rule_id kExpand2Rule = 1;
static constexpr rule_id kExpand1Rule = 2;

struct MockGoalActivator : public i_goal_activator {
    MOCK_METHOD(void, activate, (const goal_lineage*), (override));
};

struct MockGetRule : public i_get_rule {
    MOCK_METHOD(const rule*, get, (rule_id), (const, override));
};

struct MockActivateGoalCandidates : public i_activate_goal_candidates {
    MOCK_METHOD(bool, activate_goal_candidates, (const goal_lineage*), (override));
};

struct MockGoalDeactivator : public i_goal_deactivator {
    MOCK_METHOD(void, deactivate, (const goal_lineage*), (override));
};

struct MockDeactivateGoalCandidates : public i_deactivate_goal_candidates {
    MOCK_METHOD(void, deactivate_goal_candidates, (const goal_lineage*), (override));
};

template<typename Yield>
std::vector<Yield> collect_yields(coroutine<Yield, void>& sm) {
    std::vector<Yield> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            out.push_back(sm.consume_yield());
    }
    return out;
}

struct SrtTreeSnapshot {
    std::vector<const goal_lineage*> sorted_roots;
    std::map<const goal_lineage*, std::vector<const goal_lineage*>> sorted_edges;
    std::vector<const goal_lineage*> sorted_leaves;

    bool operator==(const SrtTreeSnapshot& other) const {
        return sorted_roots == other.sorted_roots
            && sorted_leaves == other.sorted_leaves
            && sorted_edges == other.sorted_edges;
    }
};

std::string format_snapshot(const SrtTreeSnapshot& snap) {
    std::ostringstream os;
    os << "roots=[";
    for (size_t i = 0; i < snap.sorted_roots.size(); ++i) {
        if (i)
            os << ',';
        os << snap.sorted_roots[i];
    }
    os << "] leaves=[";
    for (size_t i = 0; i < snap.sorted_leaves.size(); ++i) {
        if (i)
            os << ',';
        os << snap.sorted_leaves[i];
    }
    os << "] edges={";
    bool first_edge = true;
    for (const auto& [parent, children] : snap.sorted_edges) {
        if (!first_edge)
            os << ' ';
        first_edge = false;
        os << parent << "->[";
        for (size_t i = 0; i < children.size(); ++i) {
            if (i)
                os << ',';
            os << children[i];
        }
        os << ']';
    }
    os << '}';
    return os.str();
}

void collect_subtree(
    const goal_lineage* node,
    bool is_root,
    i_iterate_child_goals& iterate_children,
    i_is_active_goal& is_active,
    SrtTreeSnapshot& snap) {
    if (is_active.is_active_goal(node))
        snap.sorted_leaves.push_back(node);

    const bool isolated_root = is_root && is_active.is_active_goal(node);
    if (isolated_root)
        return;

    // Active goals are SRT leaves; they have no children row in the tree.
    if (is_active.is_active_goal(node))
        return;

    auto child_sm = iterate_children.iterate_child_goals(node);
    std::vector<const goal_lineage*> children = collect_yields(child_sm);
    if (children.empty())
        return;

    std::sort(children.begin(), children.end());
    snap.sorted_edges[node] = children;
    for (const goal_lineage* child : children)
        collect_subtree(child, false, iterate_children, is_active, snap);
}

SrtTreeSnapshot snapshot_srt_tree(locator& loc) {
    auto& iterate_roots = loc.locate<i_iterate_root_goals>();
    auto& iterate_children = loc.locate<i_iterate_child_goals>();
    auto& is_active = loc.locate<i_is_active_goal>();

    SrtTreeSnapshot snap;
    auto root_sm = iterate_roots.iterate_root_goals();
    snap.sorted_roots = collect_yields(root_sm);
    std::sort(snap.sorted_roots.begin(), snap.sorted_roots.end());

    for (const goal_lineage* root : snap.sorted_roots)
        collect_subtree(root, true, iterate_children, is_active, snap);

    std::sort(snap.sorted_leaves.begin(), snap.sorted_leaves.end());
    return snap;
}

void seed_initial_goals(locator& loc, const std::vector<const goal_lineage*>& initial) {
    auto& insert = loc.locate<i_insert_active_goal>();
    auto& flush = loc.locate<i_srt_flush_goal_batch>();
    for (const goal_lineage* gl : initial) {
        insert.insert_active_goal(gl);
        flush.flush_srt_goal_batch();
    }
}

void expect_snapshots_equal(
    const SrtTreeSnapshot& expected,
    const SrtTreeSnapshot& actual) {
    EXPECT_EQ(expected.sorted_roots, actual.sorted_roots);
    EXPECT_EQ(expected.sorted_leaves, actual.sorted_leaves);
    EXPECT_EQ(expected.sorted_edges, actual.sorted_edges)
        << "expected " << format_snapshot(expected)
        << " actual " << format_snapshot(actual);
}

void expect_snapshots_not_equal(
    const SrtTreeSnapshot& reference,
    const SrtTreeSnapshot& actual) {
    EXPECT_FALSE(reference == actual)
        << "reference " << format_snapshot(reference)
        << " actual " << format_snapshot(actual);
}

void expect_identical_trees_for_orders(
    locator& loc,
    resolver& res,
    const std::vector<const goal_lineage*>& initial_goals,
    const std::vector<const resolution_lineage*>& steps,
    const std::vector<std::vector<size_t>>& orders) {
    ASSERT_FALSE(orders.empty());
    std::vector<SrtTreeSnapshot> snapshots;
    snapshots.reserve(orders.size());

    auto& clear = loc.locate<i_clear_active_goals>();
    for (const std::vector<size_t>& order : orders) {
        clear.clear_active_goals();
        seed_initial_goals(loc, initial_goals);
        for (size_t step_idx : order)
            ASSERT_TRUE(res.resolve(steps[step_idx]));
        snapshots.push_back(snapshot_srt_tree(loc));
    }

    for (size_t i = 1; i < snapshots.size(); ++i)
        expect_snapshots_equal(snapshots[0], snapshots[i]);
}

std::vector<std::vector<size_t>> permute_ground_steps(
    size_t expand_step_count,
    size_t ground_step_count) {
    std::vector<size_t> ground_indices(ground_step_count);
    std::iota(ground_indices.begin(), ground_indices.end(), expand_step_count);

    std::vector<std::vector<size_t>> orders;
    do {
        std::vector<size_t> order;
        order.reserve(expand_step_count + ground_step_count);
        for (size_t i = 0; i < expand_step_count; ++i)
            order.push_back(i);
        order.insert(order.end(), ground_indices.begin(), ground_indices.end());
        orders.push_back(std::move(order));
    } while (std::next_permutation(ground_indices.begin(), ground_indices.end()));

    return orders;
}

}  // namespace

class SrtResolverOrderInvarianceIntegrationTest : public ::testing::Test {
protected:
    locator loc;
    lineage_pool pool;
    srt_active_goals active_goals;

    expr head_{expr::var{0}};
    expr body0_{expr::var{1}};
    expr body1_{expr::var{2}};
    rule ground_rule_{&head_, {}};
    rule expand2_rule_{&head_, {&body0_, &body1_}};
    rule expand1_rule_{&head_, {&body0_}};

    testing::NiceMock<MockGoalActivator> goal_activator;
    testing::NiceMock<MockGetRule> get_rule;
    testing::NiceMock<MockActivateGoalCandidates> activate_candidates;
    testing::NiceMock<MockGoalDeactivator> goal_deactivator;
    testing::NiceMock<MockDeactivateGoalCandidates> deactivate_candidates;

    std::unique_ptr<subgoals_activator> subgoals;
    std::unique_ptr<srt_subgoals_activator> srt_subgoals;
    std::unique_ptr<resolver> res;

    void SetUp() override {
        loc.bind_as<i_make_goal_lineage, i_make_resolution_lineage>(pool);
        loc.bind_as<
            i_insert_active_goal,
            i_srt_link_goal_batch_parent,
            i_srt_flush_goal_batch,
            i_clear_active_goals,
            i_iterate_root_goals,
            i_iterate_child_goals,
            i_is_active_goal,
            i_active_goals_size,
            i_check_active_goals_empty>(active_goals);

        loc.bind_as<i_goal_activator>(goal_activator);
        loc.bind_as<i_get_rule>(get_rule);
        loc.bind_as<i_activate_goal_candidates>(activate_candidates);
        loc.bind_as<i_goal_deactivator>(goal_deactivator);
        loc.bind_as<i_deactivate_goal_candidates>(deactivate_candidates);

        ON_CALL(goal_activator, activate(testing::_))
            .WillByDefault([this](const goal_lineage* gl) {
                loc.locate<i_insert_active_goal>().insert_active_goal(gl);
            });

        ON_CALL(get_rule, get(testing::_))
            .WillByDefault([this](rule_id id) -> const rule* {
                switch (id) {
                case kGroundRule:
                    return &ground_rule_;
                case kExpand2Rule:
                    return &expand2_rule_;
                case kExpand1Rule:
                    return &expand1_rule_;
                default:
                    return nullptr;
                }
            });

        ON_CALL(activate_candidates, activate_goal_candidates(testing::_))
            .WillByDefault(Return(true));

        subgoals = std::make_unique<subgoals_activator>(loc);
        loc.bind_as<>(*subgoals);

        srt_subgoals = std::make_unique<srt_subgoals_activator>(loc);
        loc.bind_as<i_activate_subgoals_and_candidates>(*srt_subgoals);

        res = std::make_unique<resolver>(loc);
    }
};

TEST_F(SrtResolverOrderInvarianceIntegrationTest, IndependentRootsGroundedInEitherOrder) {
    /*
     * Intent: two isolated initial roots grounded in either order → empty forest.
     * Validity: both steps are independent ground resolutions on distinct roots.
     */
    const goal_lineage* root_f = pool.make_goal_lineage(nullptr, 0);
    const goal_lineage* root_g = pool.make_goal_lineage(nullptr, 1);
    const resolution_lineage* rl_ground_f = pool.make_resolution_lineage(root_f, kGroundRule);
    const resolution_lineage* rl_ground_g = pool.make_resolution_lineage(root_g, kGroundRule);

    const std::vector<const goal_lineage*> initial{root_f, root_g};
    const std::vector<const resolution_lineage*> steps{rl_ground_f, rl_ground_g};
    const std::vector<std::vector<size_t>> orders{{0, 1}, {1, 0}};

    expect_identical_trees_for_orders(loc, *res, initial, steps, orders);
    EXPECT_TRUE(loc.locate<i_check_active_goals_empty>().empty());
}

TEST_F(SrtResolverOrderInvarianceIntegrationTest, NestedLeavesGroundedInAllPermutations) {
    /*
     * Intent: expand h→{f,g}, expand f→{f0,f1}, then ground {g,f0,f1} in all 3! orders.
     * Validity: two fixed expands first; only leaf ground steps commute.
     */
    const goal_lineage* h = pool.make_goal_lineage(nullptr, 0);
    const resolution_lineage* rl_expand_h = pool.make_resolution_lineage(h, kExpand2Rule);
    const goal_lineage* f = pool.make_goal_lineage(rl_expand_h, 0);
    const goal_lineage* g = pool.make_goal_lineage(rl_expand_h, 1);
    const resolution_lineage* rl_expand_f = pool.make_resolution_lineage(f, kExpand2Rule);
    const goal_lineage* f0 = pool.make_goal_lineage(rl_expand_f, 0);
    const goal_lineage* f1 = pool.make_goal_lineage(rl_expand_f, 1);
    const resolution_lineage* rl_ground_g = pool.make_resolution_lineage(g, kGroundRule);
    const resolution_lineage* rl_ground_f0 = pool.make_resolution_lineage(f0, kGroundRule);
    const resolution_lineage* rl_ground_f1 = pool.make_resolution_lineage(f1, kGroundRule);

    const std::vector<const goal_lineage*> initial{h};
    const std::vector<const resolution_lineage*> steps{
        rl_expand_h,
        rl_expand_f,
        rl_ground_g,
        rl_ground_f0,
        rl_ground_f1,
    };
    const std::vector<std::vector<size_t>> orders = permute_ground_steps(2, 3);
    ASSERT_EQ(orders.size(), 6u);

    expect_identical_trees_for_orders(loc, *res, initial, steps, orders);
    EXPECT_TRUE(loc.locate<i_check_active_goals_empty>().empty());
}

TEST_F(SrtResolverOrderInvarianceIntegrationTest, SiblingSubgoalsGroundedInEitherOrder) {
    /*
     * Intent: expand parent→{a,b}, then ground siblings in either order → empty forest.
     * Validity: expand must precede both grounds; sibling grounds commute.
     */
    const goal_lineage* parent = pool.make_goal_lineage(nullptr, 0);
    const resolution_lineage* rl_expand = pool.make_resolution_lineage(parent, kExpand2Rule);
    const goal_lineage* child_a = pool.make_goal_lineage(rl_expand, 0);
    const goal_lineage* child_b = pool.make_goal_lineage(rl_expand, 1);
    const resolution_lineage* rl_ground_a = pool.make_resolution_lineage(child_a, kGroundRule);
    const resolution_lineage* rl_ground_b = pool.make_resolution_lineage(child_b, kGroundRule);

    const std::vector<const goal_lineage*> initial{parent};
    const std::vector<const resolution_lineage*> steps{rl_expand, rl_ground_a, rl_ground_b};
    const std::vector<std::vector<size_t>> orders{{0, 1, 2}, {0, 2, 1}};

    expect_identical_trees_for_orders(loc, *res, initial, steps, orders);
    EXPECT_TRUE(loc.locate<i_check_active_goals_empty>().empty());
}

TEST_F(SrtResolverOrderInvarianceIntegrationTest, TwoIndependentSubtreesExpandOrderInvariant) {
    /*
     * Intent: two independent roots expanded in either order, zero grounds — same partial tree.
     * Validity: multiset {expand_r1, expand_r2} identical; only expand order permuted.
     */
    const goal_lineage* root_a = pool.make_goal_lineage(nullptr, 0);
    const goal_lineage* root_b = pool.make_goal_lineage(nullptr, 1);
    const resolution_lineage* rl_expand_a = pool.make_resolution_lineage(root_a, kExpand2Rule);
    const resolution_lineage* rl_expand_b = pool.make_resolution_lineage(root_b, kExpand2Rule);
    pool.make_goal_lineage(rl_expand_a, 0);
    pool.make_goal_lineage(rl_expand_a, 1);
    pool.make_goal_lineage(rl_expand_b, 0);
    pool.make_goal_lineage(rl_expand_b, 1);

    const std::vector<const goal_lineage*> initial{root_a, root_b};
    const std::vector<const resolution_lineage*> steps{rl_expand_a, rl_expand_b};
    const std::vector<std::vector<size_t>> orders{{0, 1}, {1, 0}};

    expect_identical_trees_for_orders(loc, *res, initial, steps, orders);
    EXPECT_EQ(loc.locate<i_active_goals_size>().active_goals_size(), 4u);
}

TEST_F(SrtResolverOrderInvarianceIntegrationTest, TwoIndependentSubtreesCommutingGroundOrder) {
    /*
     * Intent: 2×2 children, 4 ground steps; commute grounds across independent subtrees.
     * Validity: both expands fixed first; full multiset of four grounds identical.
     */
    const goal_lineage* root_a = pool.make_goal_lineage(nullptr, 0);
    const goal_lineage* root_b = pool.make_goal_lineage(nullptr, 1);
    const resolution_lineage* rl_expand_a = pool.make_resolution_lineage(root_a, kExpand2Rule);
    const resolution_lineage* rl_expand_b = pool.make_resolution_lineage(root_b, kExpand2Rule);
    const goal_lineage* a0 = pool.make_goal_lineage(rl_expand_a, 0);
    const goal_lineage* a1 = pool.make_goal_lineage(rl_expand_a, 1);
    const goal_lineage* b0 = pool.make_goal_lineage(rl_expand_b, 0);
    const goal_lineage* b1 = pool.make_goal_lineage(rl_expand_b, 1);
    const resolution_lineage* rl_ground_a0 = pool.make_resolution_lineage(a0, kGroundRule);
    const resolution_lineage* rl_ground_a1 = pool.make_resolution_lineage(a1, kGroundRule);
    const resolution_lineage* rl_ground_b0 = pool.make_resolution_lineage(b0, kGroundRule);
    const resolution_lineage* rl_ground_b1 = pool.make_resolution_lineage(b1, kGroundRule);

    const std::vector<const goal_lineage*> initial{root_a, root_b};
    const std::vector<const resolution_lineage*> steps{
        rl_expand_a,
        rl_expand_b,
        rl_ground_a0,
        rl_ground_a1,
        rl_ground_b0,
        rl_ground_b1,
    };
    const std::vector<std::vector<size_t>> orders{
        {0, 1, 2, 3, 4, 5},
        {0, 1, 2, 4, 3, 5},
        {0, 1, 4, 5, 2, 3},
    };

    expect_identical_trees_for_orders(loc, *res, initial, steps, orders);
    EXPECT_TRUE(loc.locate<i_check_active_goals_empty>().empty());
}

TEST_F(SrtResolverOrderInvarianceIntegrationTest, SingleChildExpandUnaryCollapse) {
    /*
     * Intent: kExpand1Rule unary-collapses parent; ground targets child only.
     * After expand: sole active leaf is child (parent absent from forest).
     * After ground: empty forest.
     */
    const goal_lineage* parent = pool.make_goal_lineage(nullptr, 0);
    const resolution_lineage* rl_expand = pool.make_resolution_lineage(parent, kExpand1Rule);
    const goal_lineage* child = pool.make_goal_lineage(rl_expand, 0);
    const resolution_lineage* rl_ground_child = pool.make_resolution_lineage(child, kGroundRule);

    loc.locate<i_clear_active_goals>().clear_active_goals();
    seed_initial_goals(loc, {parent});
    ASSERT_TRUE(res->resolve(rl_expand));

    SrtTreeSnapshot after_expand = snapshot_srt_tree(loc);
    EXPECT_EQ(after_expand.sorted_roots, std::vector<const goal_lineage*>{child});
    EXPECT_EQ(after_expand.sorted_leaves, std::vector<const goal_lineage*>{child});
    EXPECT_TRUE(after_expand.sorted_edges.empty());

    ASSERT_TRUE(res->resolve(rl_ground_child));
    SrtTreeSnapshot after_ground = snapshot_srt_tree(loc);
    EXPECT_TRUE(after_ground.sorted_roots.empty());
    EXPECT_TRUE(after_ground.sorted_leaves.empty());
    EXPECT_TRUE(loc.locate<i_check_active_goals_empty>().empty());
}

TEST_F(SrtResolverOrderInvarianceIntegrationTest, InvalidOrderNegativeControl) {
    /*
     * Intent: ground parent before expand — parent removed, expand batch link fails silently,
     * resolve() still true, children remain orphan roots. Differs from valid expand checkpoint.
     * (Grounding inactive child_a before expand is a no-op in this harness.)
     */
    const goal_lineage* parent = pool.make_goal_lineage(nullptr, 0);
    const resolution_lineage* rl_expand = pool.make_resolution_lineage(parent, kExpand2Rule);
    const goal_lineage* child_a = pool.make_goal_lineage(rl_expand, 0);
    const goal_lineage* child_b = pool.make_goal_lineage(rl_expand, 1);
    const resolution_lineage* rl_ground_parent = pool.make_resolution_lineage(parent, kGroundRule);
    const resolution_lineage* rl_ground_a = pool.make_resolution_lineage(child_a, kGroundRule);
    const resolution_lineage* rl_ground_b = pool.make_resolution_lineage(child_b, kGroundRule);

    const std::vector<const goal_lineage*> initial{parent};

    loc.locate<i_clear_active_goals>().clear_active_goals();
    seed_initial_goals(loc, initial);
    ASSERT_TRUE(res->resolve(rl_expand));
    const SrtTreeSnapshot valid_after_expand = snapshot_srt_tree(loc);

    loc.locate<i_clear_active_goals>().clear_active_goals();
    seed_initial_goals(loc, initial);
    EXPECT_TRUE(res->resolve(rl_ground_parent));
    ASSERT_TRUE(res->resolve(rl_expand));
    const SrtTreeSnapshot invalid_after_expand = snapshot_srt_tree(loc);

    expect_snapshots_not_equal(valid_after_expand, invalid_after_expand);

    loc.locate<i_clear_active_goals>().clear_active_goals();
    seed_initial_goals(loc, initial);
    for (const resolution_lineage* step : {rl_expand, rl_ground_a, rl_ground_b})
        ASSERT_TRUE(res->resolve(step));
    EXPECT_TRUE(loc.locate<i_check_active_goals_empty>().empty());
}
