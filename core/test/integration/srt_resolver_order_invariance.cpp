// Integration: srt_subgoals_activator → subgoals_activator → srt_active_goals / series_reduced_tree.
// Invariant: for the same unordered multiset of resolutions, valid permutations of resolve order
// leave the entire SRT forest identical (checked via locator-bound i_iterate_root_goals /
// i_iterate_child_goals). resolver steps 2–3 (candidate/goal deactivators) do not touch the tree.

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <unordered_map>
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

static constexpr rule_id kFuzzGroundRuleId = 0;
static constexpr rule_id kFuzzExpand1RuleId = 1;
static constexpr rule_id kFuzzExpand2RuleId = 2;
static constexpr rule_id kFuzzExpand3RuleId = 3;

static constexpr size_t kFuzzInitialRootCount = 6;
static constexpr size_t kOuterIterations = 1000;
static constexpr size_t kMaxDrainSteps = 10'000;
static constexpr size_t kMinDrainSteps = 3;

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
    EXPECT_EQ(expected.sorted_roots, actual.sorted_roots)
        << "expected " << format_snapshot(expected)
        << " actual " << format_snapshot(actual);
    EXPECT_EQ(expected.sorted_leaves, actual.sorted_leaves)
        << "expected " << format_snapshot(expected)
        << " actual " << format_snapshot(actual);
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

    auto& clear = loc.locate<i_clear_active_goals>();
    std::optional<SrtTreeSnapshot> reference;
    for (const std::vector<size_t>& order : orders) {
        clear.clear_active_goals();
        seed_initial_goals(loc, initial_goals);
        for (size_t step_idx : order)
            ASSERT_TRUE(res.resolve(steps[step_idx]));
        SrtTreeSnapshot snap = snapshot_srt_tree(loc);
        if (!reference)
            reference = snap;
        else
            expect_snapshots_equal(*reference, snap);
    }
}

struct ResolutionScript {
    std::vector<const goal_lineage*> initial;
    std::vector<const resolution_lineage*> expands;
    std::vector<const resolution_lineage*> grounds;

    std::vector<const resolution_lineage*> all_steps() const {
        std::vector<const resolution_lineage*> steps;
        steps.reserve(expands.size() + grounds.size());
        steps.insert(steps.end(), expands.begin(), expands.end());
        steps.insert(steps.end(), grounds.begin(), grounds.end());
        return steps;
    }
};

ResolutionScript make_independent_subtrees_script(lineage_pool& pool, size_t subtree_count) {
    ResolutionScript script;
    script.initial.reserve(subtree_count);
    script.expands.reserve(subtree_count);
    script.grounds.reserve(subtree_count * 2);

    for (size_t i = 0; i < subtree_count; ++i) {
        const goal_lineage* root = pool.make_goal_lineage(nullptr, static_cast<subgoal_id>(i));
        script.initial.push_back(root);
        const resolution_lineage* rl_expand = pool.make_resolution_lineage(root, kExpand2Rule);
        script.expands.push_back(rl_expand);
        for (subgoal_id child_idx : {subgoal_id{0}, subgoal_id{1}}) {
            const goal_lineage* child = pool.make_goal_lineage(rl_expand, child_idx);
            script.grounds.push_back(pool.make_resolution_lineage(child, kGroundRule));
        }
    }
    return script;
}

ResolutionScript make_balanced_binary_tree_script(lineage_pool& pool, size_t expand_levels) {
    const goal_lineage* root = pool.make_goal_lineage(nullptr, 0);
    ResolutionScript script{
        .initial = {root},
    };

    std::vector<const goal_lineage*> frontier{root};
    for (size_t level = 0; level < expand_levels; ++level) {
        std::vector<const goal_lineage*> next;
        next.reserve(frontier.size() * 2);
        for (const goal_lineage* node : frontier) {
            const resolution_lineage* rl_expand = pool.make_resolution_lineage(node, kExpand2Rule);
            script.expands.push_back(rl_expand);
            for (subgoal_id child_idx : {subgoal_id{0}, subgoal_id{1}}) {
                const goal_lineage* child = pool.make_goal_lineage(rl_expand, child_idx);
                next.push_back(child);
            }
        }
        frontier = std::move(next);
    }

    script.grounds.reserve(frontier.size());
    for (const goal_lineage* leaf : frontier)
        script.grounds.push_back(pool.make_resolution_lineage(leaf, kGroundRule));
    return script;
}

std::vector<std::vector<size_t>> permute_all_steps(size_t step_count) {
    std::vector<size_t> indices(step_count);
    std::iota(indices.begin(), indices.end(), 0);

    std::vector<std::vector<size_t>> orders;
    do
        orders.push_back(indices);
    while (std::next_permutation(indices.begin(), indices.end()));

    return orders;
}

std::vector<const goal_lineage*> sorted_lineages(
    std::initializer_list<const goal_lineage*> ptrs) {
    std::vector<const goal_lineage*> out(ptrs.begin(), ptrs.end());
    std::sort(out.begin(), out.end());
    return out;
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

rule_id assign_rule(
    const goal_lineage* gl,
    std::unordered_map<const goal_lineage*, rule_id>& goal_rule) {
    if (auto it = goal_rule.find(gl); it != goal_rule.end())
        return it->second;
    const rule_id id = static_cast<rule_id>(
        reinterpret_cast<uintptr_t>(gl) % 4);
    goal_rule.emplace(gl, id);
    return id;
}

void fuzz_drain_to_empty(
    uint32_t order_seed,
    locator& loc,
    lineage_pool& pool,
    resolver& res,
    const std::vector<const goal_lineage*>& initial,
    const std::array<rule, 4>& rule_table,
    std::unordered_map<const goal_lineage*, rule_id>& goal_rule,
    SrtTreeSnapshot& out_snapshot,
    size_t& out_resolve_count,
    bool& out_saw_expand) {
    goal_rule.clear();
    loc.locate<i_clear_active_goals>().clear_active_goals();
    seed_initial_goals(loc, initial);

    for (const goal_lineage* gl : initial)
        assign_rule(gl, goal_rule);
    if (!initial.empty())
        goal_rule[initial[0]] = kFuzzExpand2RuleId;

    std::vector<const goal_lineage*> currently_active_goals = initial;
    out_resolve_count = 0;
    out_saw_expand = false;

    std::mt19937 order_rng(order_seed);
    size_t step_count = 0;

    while (!currently_active_goals.empty()) {
        SCOPED_TRACE(step_count);
        ASSERT_LT(step_count, kMaxDrainSteps) << "drain did not terminate";
        ++step_count;

        const size_t i = order_rng() % currently_active_goals.size();
        const goal_lineage* goal = currently_active_goals[i];
        const rule_id picked_rule = goal_rule.at(goal);
        const rule& picked = rule_table[picked_rule];

        const resolution_lineage* rl = pool.make_resolution_lineage(goal, picked_rule);
        std::vector<const goal_lineage*> children;
        children.reserve(picked.body.size());
        for (size_t c = 0; c < picked.body.size(); ++c)
            children.push_back(pool.make_goal_lineage(rl, static_cast<subgoal_id>(c)));

        ASSERT_TRUE(res.resolve(rl));
        ++out_resolve_count;
        if (!children.empty())
            out_saw_expand = true;

        currently_active_goals[i] = currently_active_goals.back();
        currently_active_goals.pop_back();

        for (const goal_lineage* child : children) {
            assign_rule(child, goal_rule);
            currently_active_goals.push_back(child);
        }
    }

    out_snapshot = snapshot_srt_tree(loc);
}

}  // namespace

struct SrtResolverOrderInvarianceIntegrationTest : public ::testing::Test {
    locator loc;
    lineage_pool pool;
    srt_active_goals active_goals;

    expr head_{expr::var{0}};
    expr body0_{expr::var{1}};
    expr body1_{expr::var{2}};
    expr body2_{expr::var{3}};
    rule ground_rule_{&head_, {}};
    rule expand2_rule_{&head_, {&body0_, &body1_}};
    rule expand1_rule_{&head_, {&body0_}};
    rule expand3_rule_{&head_, {&body0_, &body1_, &body2_}};

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
    const goal_lineage* a0 = pool.make_goal_lineage(rl_expand_a, 0);
    const goal_lineage* a1 = pool.make_goal_lineage(rl_expand_a, 1);
    const goal_lineage* b0 = pool.make_goal_lineage(rl_expand_b, 0);
    const goal_lineage* b1 = pool.make_goal_lineage(rl_expand_b, 1);

    const std::vector<const goal_lineage*> initial{root_a, root_b};
    const std::vector<const resolution_lineage*> steps{rl_expand_a, rl_expand_b};
    const std::vector<std::vector<size_t>> orders{{0, 1}, {1, 0}};

    expect_identical_trees_for_orders(loc, *res, initial, steps, orders);
    EXPECT_EQ(loc.locate<i_active_goals_size>().active_goals_size(), 4u);

    loc.locate<i_clear_active_goals>().clear_active_goals();
    seed_initial_goals(loc, initial);
    ASSERT_TRUE(res->resolve(rl_expand_a));
    ASSERT_TRUE(res->resolve(rl_expand_b));
    SrtTreeSnapshot expected{
        .sorted_roots = sorted_lineages({root_a, root_b}),
        .sorted_edges = {
            {root_a, sorted_lineages({a0, a1})},
            {root_b, sorted_lineages({b0, b1})},
        },
        .sorted_leaves = sorted_lineages({a0, a1, b0, b1}),
    };
    expect_snapshots_equal(expected, snapshot_srt_tree(loc));
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
    const std::vector<std::vector<size_t>> orders = permute_ground_steps(2, 4);
    ASSERT_EQ(orders.size(), 24u);

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

TEST_F(SrtResolverOrderInvarianceIntegrationTest, PartialSiblingGroundingDiffers) {
    /*
     * Intent: after expand, {expand, ground_a} vs {expand, ground_b} differ — one sibling remains.
     * Validity: partial multisets differ; order matters mid-run (complement to invariance tests).
     */
    const goal_lineage* parent = pool.make_goal_lineage(nullptr, 0);
    const resolution_lineage* rl_expand = pool.make_resolution_lineage(parent, kExpand2Rule);
    const goal_lineage* child_a = pool.make_goal_lineage(rl_expand, 0);
    const goal_lineage* child_b = pool.make_goal_lineage(rl_expand, 1);
    const resolution_lineage* rl_ground_a = pool.make_resolution_lineage(child_a, kGroundRule);
    const resolution_lineage* rl_ground_b = pool.make_resolution_lineage(child_b, kGroundRule);

    const std::vector<const goal_lineage*> initial{parent};

    loc.locate<i_clear_active_goals>().clear_active_goals();
    seed_initial_goals(loc, initial);
    ASSERT_TRUE(res->resolve(rl_expand));
    ASSERT_TRUE(res->resolve(rl_ground_a));
    const SrtTreeSnapshot ground_a_first = snapshot_srt_tree(loc);

    loc.locate<i_clear_active_goals>().clear_active_goals();
    seed_initial_goals(loc, initial);
    ASSERT_TRUE(res->resolve(rl_expand));
    ASSERT_TRUE(res->resolve(rl_ground_b));
    const SrtTreeSnapshot ground_b_first = snapshot_srt_tree(loc);

    expect_snapshots_not_equal(ground_a_first, ground_b_first);
    EXPECT_EQ(ground_a_first.sorted_leaves, std::vector<const goal_lineage*>{child_b});
    EXPECT_EQ(ground_b_first.sorted_leaves, std::vector<const goal_lineage*>{child_a});
}

TEST_F(SrtResolverOrderInvarianceIntegrationTest, NestedPartialExpandCheckpoint) {
    /*
     * Intent: after {expand_h, expand_f} only — branching root h, internal f, leaves g,f0,f1.
     * Validity: guards expand-order wiring before ground permutations in nested test.
     */
    const goal_lineage* h = pool.make_goal_lineage(nullptr, 0);
    const resolution_lineage* rl_expand_h = pool.make_resolution_lineage(h, kExpand2Rule);
    const goal_lineage* f = pool.make_goal_lineage(rl_expand_h, 0);
    const goal_lineage* g = pool.make_goal_lineage(rl_expand_h, 1);
    const resolution_lineage* rl_expand_f = pool.make_resolution_lineage(f, kExpand2Rule);
    const goal_lineage* f0 = pool.make_goal_lineage(rl_expand_f, 0);
    const goal_lineage* f1 = pool.make_goal_lineage(rl_expand_f, 1);

    loc.locate<i_clear_active_goals>().clear_active_goals();
    seed_initial_goals(loc, {h});
    ASSERT_TRUE(res->resolve(rl_expand_h));
    ASSERT_TRUE(res->resolve(rl_expand_f));

    SrtTreeSnapshot expected{
        .sorted_roots = {h},
        .sorted_edges = {
            {h, sorted_lineages({f, g})},
            {f, sorted_lineages({f0, f1})},
        },
        .sorted_leaves = sorted_lineages({f0, f1, g}),
    };
    expect_snapshots_equal(expected, snapshot_srt_tree(loc));
    EXPECT_EQ(loc.locate<i_active_goals_size>().active_goals_size(), 3u);
}

TEST_F(SrtResolverOrderInvarianceIntegrationTest, InvalidOrderNegativeControl) {
    /*
     * Intent: plan's child-before-expand is vacuous here (inactive child ground is a no-op);
     * we use parent-before-expand instead — parent removed, expand batch link fails silently,
     * resolve() still true, children remain orphan roots. Differs from valid expand checkpoint.
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
    const SrtTreeSnapshot seeded = snapshot_srt_tree(loc);
    EXPECT_TRUE(res->resolve(rl_ground_a));
    expect_snapshots_equal(seeded, snapshot_srt_tree(loc));

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

TEST_F(SrtResolverOrderInvarianceIntegrationTest, FourIndependentSubtreesStressFullGroundPermutations) {
    /*
     * Stress: 4 disjoint expand2 subtrees → 8 leaf grounds in all 8! = 40320 valid orders.
     * ~500k resolves; catches cross-subtree ground commuting at scale.
     */
    const ResolutionScript script = make_independent_subtrees_script(pool, 4);
    const std::vector<const resolution_lineage*> steps = script.all_steps();
    const std::vector<std::vector<size_t>> orders = permute_ground_steps(
        script.expands.size(), script.grounds.size());
    ASSERT_EQ(orders.size(), 40320u);

    expect_identical_trees_for_orders(loc, *res, script.initial, steps, orders);
    EXPECT_TRUE(loc.locate<i_check_active_goals_empty>().empty());
}

TEST_F(SrtResolverOrderInvarianceIntegrationTest, BalancedBinaryTreeStressFullGroundPermutations) {
    /*
     * Stress: 3 expand levels (7 expands, 8 leaves) then all 8! = 40320 ground orders.
     * Exercises deep unary/nullary reduction chains under heavy permutation load.
     */
    const ResolutionScript script = make_balanced_binary_tree_script(pool, 3);
    ASSERT_EQ(script.expands.size(), 7u);
    ASSERT_EQ(script.grounds.size(), 8u);

    const std::vector<const resolution_lineage*> steps = script.all_steps();
    const std::vector<std::vector<size_t>> orders = permute_ground_steps(
        script.expands.size(), script.grounds.size());
    ASSERT_EQ(orders.size(), 40320u);

    expect_identical_trees_for_orders(loc, *res, script.initial, steps, orders);
    EXPECT_TRUE(loc.locate<i_check_active_goals_empty>().empty());
}

TEST_F(SrtResolverOrderInvarianceIntegrationTest, EightIndependentRootsStressExpandOrderInvariant) {
    /*
     * Stress: 8 isolated roots expanded in all 8! = 40320 orders (zero grounds).
     * ~320k resolves on a 16-leaf partial forest; expand-only commuting at scale.
     */
    const ResolutionScript script = make_independent_subtrees_script(pool, 8);
    const std::vector<std::vector<size_t>> orders = permute_all_steps(script.expands.size());
    ASSERT_EQ(orders.size(), 40320u);

    expect_identical_trees_for_orders(loc, *res, script.initial, script.expands, orders);
    EXPECT_EQ(loc.locate<i_active_goals_size>().active_goals_size(), 16u);
}

TEST_F(SrtResolverOrderInvarianceIntegrationTest, RandomResolutionFuzzOrderInvariantTwoSeeds) {
    const std::array<rule, 4> rule_table{
        ground_rule_,
        expand1_rule_,
        expand2_rule_,
        expand3_rule_,
    };
    ON_CALL(get_rule, get(testing::_))
        .WillByDefault([&rule_table](rule_id id) -> const rule* {
            return id < rule_table.size() ? &rule_table[id] : nullptr;
        });

    // lineage_pool interns are stable across outer iterations; pool carry-over is intentional.
    std::vector<const goal_lineage*> initial;
    initial.reserve(kFuzzInitialRootCount);
    for (size_t i = 0; i < kFuzzInitialRootCount; ++i)
        initial.push_back(pool.make_goal_lineage(nullptr, static_cast<subgoal_id>(i)));

    std::mt19937 outer_rng(0xC0FFEE);
    std::unordered_map<const goal_lineage*, rule_id> goal_rule;

    for (size_t outer = 0; outer < kOuterIterations; ++outer) {
        SCOPED_TRACE(outer);

        uint32_t seed_a = outer_rng();
        uint32_t seed_b = outer_rng();
        if (seed_b == seed_a)
            ++seed_b;

        size_t count_a = 0;
        size_t count_b = 0;
        bool expand_a = false;
        bool expand_b = false;

        SrtTreeSnapshot final_a;
        SrtTreeSnapshot final_b;
        fuzz_drain_to_empty(
            seed_a, loc, pool, *res, initial, rule_table, goal_rule,
            final_a, count_a, expand_a);
        fuzz_drain_to_empty(
            seed_b, loc, pool, *res, initial, rule_table, goal_rule,
            final_b, count_b, expand_b);

        expect_snapshots_equal(final_a, final_b);
        ASSERT_EQ(count_a, count_b);
        ASSERT_GE(count_a, kMinDrainSteps);
        ASSERT_TRUE(expand_a);
    }
}
