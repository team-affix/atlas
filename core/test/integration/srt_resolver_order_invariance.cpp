// Integration: srt_subgoals_activator → subgoals_activator → srt_active_goals / series_reduced_tree.
// Invariant: for the same unordered multiset of resolutions, valid permutations of resolve order
// leave the entire SRT forest identical (checked via locator-bound i_iterate_root_goals /
// i_iterate_child_goals). resolver steps 2–3 (candidate/goal deactivators) do not touch the tree.

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
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
#include "infrastructure/resolver.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "infrastructure/srt_subgoals_activator.hpp"
#include "infrastructure/subgoals_activator.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

using ::testing::Return;

namespace {

static constexpr rule_id kGroundRule = 0;
static constexpr rule_id kExpand1Rule = 1;
static constexpr rule_id kExpand2Rule = 2;
static constexpr rule_id kExpand3Rule = 3;

static constexpr size_t kFuzzInitialRootCount = 6;
static constexpr size_t kOuterIterations = 1000;
static constexpr size_t kMaxDrainSteps = 10'000;
static constexpr size_t kMinDrainSteps = 3;

struct MockGoalActivator {
    MOCK_METHOD(void, activate, (const goal_lineage*));
};

struct MockGetRule {
    MOCK_METHOD(const rule*, get_rule, (rule_id), (const));
};

struct MockActivateGoalCandidates {
    MOCK_METHOD(bool, activate_goal_candidates, (const goal_lineage*));
};

struct MockGoalDeactivator {
    MOCK_METHOD(void, deactivate, (const goal_lineage*));
};

struct MockDeactivateGoalCandidates {
    MOCK_METHOD(void, deactivate_goal_candidates, (const goal_lineage*));
};

struct MockSetChosenGoalCandidate {
    MOCK_METHOD(void, set, (const goal_lineage*, rule_id));
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

using test_subgoals_activator_t    = subgoals_activator<lineage_pool, MockGoalActivator, MockGetRule, MockActivateGoalCandidates>;
using test_srt_subgoals_activator_t = srt_subgoals_activator<
    srt_active_goals, srt_active_goals, test_subgoals_activator_t>;
using test_resolver_t             = resolver<MockGoalDeactivator, test_srt_subgoals_activator_t,
                                          MockDeactivateGoalCandidates, MockSetChosenGoalCandidate>;

void collect_subtree(
    const goal_lineage* node,
    bool is_root,
    srt_active_goals& srt,
    SrtTreeSnapshot& snap) {
    if (srt.is_active_goal(node))
        snap.sorted_leaves.push_back(node);

    const bool isolated_root = is_root && srt.is_active_goal(node);
    if (isolated_root)
        return;

    // Active goals are SRT leaves; they have no children row in the tree.
    if (srt.is_active_goal(node))
        return;

    auto child_sm = srt.iterate_child_goals(node);
    std::vector<const goal_lineage*> children = collect_yields(child_sm);
    if (children.empty())
        return;

    std::sort(children.begin(), children.end());
    snap.sorted_edges[node] = children;
    for (const goal_lineage* child : children)
        collect_subtree(child, false, srt, snap);
}

SrtTreeSnapshot snapshot_srt_tree(srt_active_goals& active_goals) {
    SrtTreeSnapshot snap;
    auto root_sm = active_goals.iterate_root_goals();
    snap.sorted_roots = collect_yields(root_sm);
    std::sort(snap.sorted_roots.begin(), snap.sorted_roots.end());

    for (const goal_lineage* root : snap.sorted_roots)
        collect_subtree(root, true, active_goals, snap);

    std::sort(snap.sorted_leaves.begin(), snap.sorted_leaves.end());
    return snap;
}

void seed_initial_goals(srt_active_goals& active_goals, const std::vector<const goal_lineage*>& initial) {
    for (const goal_lineage* gl : initial) {
        active_goals.insert_active_goal(gl);
        active_goals.flush_srt_goal_batch();
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
    srt_active_goals& active_goals,
    test_resolver_t& res,
    const std::vector<const goal_lineage*>& initial_goals,
    const std::vector<const resolution_lineage*>& steps,
    const std::vector<std::vector<size_t>>& orders) {
    ASSERT_FALSE(orders.empty());

    std::optional<SrtTreeSnapshot> reference;
    for (const std::vector<size_t>& order : orders) {
        active_goals.clear_active_goals();
        seed_initial_goals(active_goals, initial_goals);
        for (size_t step_idx : order)
            ASSERT_TRUE(res.resolve(steps[step_idx]));
        SrtTreeSnapshot snap = snapshot_srt_tree(active_goals);
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
    size_t h = gl->idx;
    if (gl->parent)
        h ^= std::hash<const void*>{}(gl->parent);
    const rule_id id = static_cast<rule_id>(h % 4);
    goal_rule.emplace(gl, id);
    return id;
}

struct DrainResult {
    SrtTreeSnapshot snap;
    size_t resolves = 0;
};

void fuzz_drain_to_empty(
    uint32_t order_seed,
    srt_active_goals& active_goals,
    lineage_pool& pool,
    test_resolver_t& res,
    const std::vector<const goal_lineage*>& initial,
    const std::array<rule, 4>& rule_table,
    std::unordered_map<const goal_lineage*, rule_id>& goal_rule,
    DrainResult& out) {
    goal_rule.clear();
    active_goals.clear_active_goals();
    seed_initial_goals(active_goals, initial);

    for (const goal_lineage* gl : initial)
        assign_rule(gl, goal_rule);
    if (!initial.empty())
        goal_rule[initial[0]] = kExpand2Rule;

    std::vector<const goal_lineage*> currently_active_goals = initial;
    out = {};

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
        ++out.resolves;

        currently_active_goals[i] = currently_active_goals.back();
        currently_active_goals.pop_back();

        for (const goal_lineage* child : children) {
            assign_rule(child, goal_rule);
            currently_active_goals.push_back(child);
        }
    }

    out.snap = snapshot_srt_tree(active_goals);
}

}  // namespace

struct SrtResolverOrderInvarianceIntegrationTest : public ::testing::Test {
    lineage_pool pool;
    srt_active_goals active_goals;

    expr head_{expr::var{0}};
    expr body0_{expr::var{1}};
    expr body1_{expr::var{2}};
    expr body2_{expr::var{3}};
    rule ground_rule_{&head_, {}};
    rule expand1_rule_{&head_, {&body0_}};
    rule expand2_rule_{&head_, {&body0_, &body1_}};
    rule expand3_rule_{&head_, {&body0_, &body1_, &body2_}};
    std::array<rule, 4> rule_table_{
        ground_rule_,
        expand1_rule_,
        expand2_rule_,
        expand3_rule_,
    };

    testing::NiceMock<MockGoalActivator> goal_activator;
    testing::NiceMock<MockGetRule> get_rule;
    testing::NiceMock<MockActivateGoalCandidates> activate_candidates;
    testing::NiceMock<MockGoalDeactivator> goal_deactivator;
    testing::NiceMock<MockDeactivateGoalCandidates> deactivate_candidates;
    testing::NiceMock<MockSetChosenGoalCandidate> set_chosen;

    std::unique_ptr<test_subgoals_activator_t> subgoals;
    std::unique_ptr<test_srt_subgoals_activator_t> srt_subgoals;
    std::unique_ptr<test_resolver_t> res;

    void SetUp() override {
        ON_CALL(goal_activator, activate(testing::_))
            .WillByDefault([this](const goal_lineage* gl) {
                active_goals.insert_active_goal(gl);
            });

        ON_CALL(get_rule, get_rule(testing::_))
            .WillByDefault([this](rule_id id) -> const rule* {
                return id < rule_table_.size() ? &rule_table_[id] : nullptr;
            });

        ON_CALL(activate_candidates, activate_goal_candidates(testing::_))
            .WillByDefault(Return(true));

        subgoals = std::make_unique<test_subgoals_activator_t>(pool, goal_activator, get_rule, activate_candidates);
        srt_subgoals = std::make_unique<test_srt_subgoals_activator_t>(
            active_goals, active_goals, *subgoals);
        res = std::make_unique<test_resolver_t>(goal_deactivator, *srt_subgoals, deactivate_candidates,
                                             set_chosen);
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

    expect_identical_trees_for_orders(active_goals, *res, initial, steps, orders);
    EXPECT_TRUE(active_goals.empty());
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

    expect_identical_trees_for_orders(active_goals, *res, initial, steps, orders);
    EXPECT_TRUE(active_goals.empty());
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

    expect_identical_trees_for_orders(active_goals, *res, initial, steps, orders);
    EXPECT_TRUE(active_goals.empty());
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

    expect_identical_trees_for_orders(active_goals, *res, initial, steps, orders);
    EXPECT_EQ(active_goals.active_goals_size(), 4u);

    active_goals.clear_active_goals();
    seed_initial_goals(active_goals, initial);
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
    expect_snapshots_equal(expected, snapshot_srt_tree(active_goals));
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

    expect_identical_trees_for_orders(active_goals, *res, initial, steps, orders);
    EXPECT_TRUE(active_goals.empty());
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

    active_goals.clear_active_goals();
    seed_initial_goals(active_goals, {parent});
    ASSERT_TRUE(res->resolve(rl_expand));

    SrtTreeSnapshot after_expand = snapshot_srt_tree(active_goals);
    EXPECT_EQ(after_expand.sorted_roots, std::vector<const goal_lineage*>{child});
    EXPECT_EQ(after_expand.sorted_leaves, std::vector<const goal_lineage*>{child});
    EXPECT_TRUE(after_expand.sorted_edges.empty());

    ASSERT_TRUE(res->resolve(rl_ground_child));
    SrtTreeSnapshot after_ground = snapshot_srt_tree(active_goals);
    EXPECT_TRUE(after_ground.sorted_roots.empty());
    EXPECT_TRUE(after_ground.sorted_leaves.empty());
    EXPECT_TRUE(active_goals.empty());
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

    active_goals.clear_active_goals();
    seed_initial_goals(active_goals, initial);
    ASSERT_TRUE(res->resolve(rl_expand));
    ASSERT_TRUE(res->resolve(rl_ground_a));
    const SrtTreeSnapshot ground_a_first = snapshot_srt_tree(active_goals);

    active_goals.clear_active_goals();
    seed_initial_goals(active_goals, initial);
    ASSERT_TRUE(res->resolve(rl_expand));
    ASSERT_TRUE(res->resolve(rl_ground_b));
    const SrtTreeSnapshot ground_b_first = snapshot_srt_tree(active_goals);

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

    active_goals.clear_active_goals();
    seed_initial_goals(active_goals, {h});
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
    expect_snapshots_equal(expected, snapshot_srt_tree(active_goals));
    EXPECT_EQ(active_goals.active_goals_size(), 3u);
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

    active_goals.clear_active_goals();
    seed_initial_goals(active_goals, initial);
    const SrtTreeSnapshot seeded = snapshot_srt_tree(active_goals);
    EXPECT_TRUE(res->resolve(rl_ground_a));
    expect_snapshots_equal(seeded, snapshot_srt_tree(active_goals));

    active_goals.clear_active_goals();
    seed_initial_goals(active_goals, initial);
    ASSERT_TRUE(res->resolve(rl_expand));
    const SrtTreeSnapshot valid_after_expand = snapshot_srt_tree(active_goals);

    active_goals.clear_active_goals();
    seed_initial_goals(active_goals, initial);
    EXPECT_TRUE(res->resolve(rl_ground_parent));
    ASSERT_TRUE(res->resolve(rl_expand));
    const SrtTreeSnapshot invalid_after_expand = snapshot_srt_tree(active_goals);

    expect_snapshots_not_equal(valid_after_expand, invalid_after_expand);

    active_goals.clear_active_goals();
    seed_initial_goals(active_goals, initial);
    for (const resolution_lineage* step : {rl_expand, rl_ground_a, rl_ground_b})
        ASSERT_TRUE(res->resolve(step));
    EXPECT_TRUE(active_goals.empty());
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

    expect_identical_trees_for_orders(active_goals, *res, script.initial, steps, orders);
    EXPECT_TRUE(active_goals.empty());
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

    expect_identical_trees_for_orders(active_goals, *res, script.initial, steps, orders);
    EXPECT_TRUE(active_goals.empty());
}

TEST_F(SrtResolverOrderInvarianceIntegrationTest, EightIndependentRootsStressExpandOrderInvariant) {
    /*
     * Stress: 8 isolated roots expanded in all 8! = 40320 orders (zero grounds).
     * ~320k resolves on a 16-leaf partial forest; expand-only commuting at scale.
     */
    const ResolutionScript script = make_independent_subtrees_script(pool, 8);
    const std::vector<std::vector<size_t>> orders = permute_all_steps(script.expands.size());
    ASSERT_EQ(orders.size(), 40320u);

    expect_identical_trees_for_orders(active_goals, *res, script.initial, script.expands, orders);
    EXPECT_EQ(active_goals.active_goals_size(), 16u);
}

TEST_F(SrtResolverOrderInvarianceIntegrationTest, RandomResolutionFuzzOrderInvariantTwoSeeds) {
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

        DrainResult result_a;
        DrainResult result_b;
        fuzz_drain_to_empty(
            seed_a, active_goals, pool, *res, initial, rule_table_, goal_rule, result_a);
        fuzz_drain_to_empty(
            seed_b, active_goals, pool, *res, initial, rule_table_, goal_rule, result_b);

        expect_snapshots_equal(result_a.snap, result_b.snap);
        ASSERT_EQ(result_a.resolves, result_b.resolves);
        ASSERT_GE(result_a.resolves, kMinDrainSteps);
    }
}
