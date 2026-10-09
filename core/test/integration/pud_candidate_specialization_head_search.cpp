#include <algorithm>
#include <deque>
#include <optional>
#include <queue>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_candidate_specialization_head.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_witness_search_head_factory.hpp"
#include "infrastructure/pud_witness_search_head_forker.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::UnorderedElementsAre;

using child_iter = std::vector<pud_node_id>::const_iterator;

struct descent_t {
    pud_node_id node;
    bool operator==(const descent_t&) const = default;
};

struct MockCheckLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_node_id));
};

struct MockGetChildren {
    MOCK_METHOD((const std::vector<pud_node_id>&), get, (pud_node_id));
};

struct MockDescend {
    MOCK_METHOD(std::optional<descent_t>, descend, (descent_t, pud_node_id));
};

struct MockCallSite {
    MOCK_METHOD(size_t, get, (pud_node_id));
};

using witness_head_t    = pud_witness_search_head<descent_t, child_iter, MockCheckLeaf, MockGetChildren, MockDescend, MockCallSite>;
using witness_factory_t = pud_witness_search_head_factory<descent_t, child_iter, MockCheckLeaf, MockGetChildren, MockDescend, MockCallSite>;
using witness_forker_t  = pud_witness_search_head_forker<descent_t, child_iter, MockCheckLeaf, MockGetChildren, MockDescend, MockCallSite>;
using mhws_t            = pud_mhws<descent_t, child_iter, witness_head_t, witness_factory_t, witness_forker_t>;
using candidate_head_t  = pud_candidate_specialization_head<
    descent_t, child_iter, mhws_t, mhws_t, mhws_t, MockCheckLeaf, MockGetChildren, MockDescend>;

using optional_context_t = std::optional<pud_candidate_resume_context<descent_t>>;

// What the shallowest-choice-point search should answer for a given tree and
// set of live leaves, computed directly from the tree (the oracle).
enum class outcome_kind { none, self_witness, choice_point };

struct expected_outcome {
    outcome_kind kind;
    pud_node_id node;
};

// A candidate head plus the bookkeeping needed to route refuted witnesses to it.
struct tracked_head {
    candidate_head_t head;
    std::vector<pud_mhws_head_id> owned;
    bool finished;
};

constexpr uint32_t k_chain_depth = 2000;
constexpr uint32_t k_wide_child_count = 5000;
constexpr uint32_t k_shared_head_count = 300;
constexpr uint32_t k_random_tree_count = 200;
constexpr uint32_t k_random_tree_max_nodes = 300;
constexpr uint32_t k_random_tree_max_depth = 12;

struct PudCandidateSpecializationHeadSearchIntegrationTest : public ::testing::Test {
    NiceMock<MockCheckLeaf>   leaves;
    NiceMock<MockGetChildren> children;
    NiceMock<MockDescend>     descend;
    NiceMock<MockCallSite>    call_sites;
    witness_factory_t witness_factory{leaves, children, descend, call_sites};
    witness_forker_t  witness_forker;
    mhws_t            witnesses{witness_factory, witness_forker};
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> sequences;
    std::unordered_set<pud_node_id> live_leaves;
    std::unordered_set<pud_node_id> blocked_nodes;
    pud_node_id root    = 1;
    pud_node_id left    = 2;
    pud_node_id right   = 3;
    pud_node_id left_1  = 4;
    pud_node_id left_2  = 5;
    pud_node_id right_1 = 6;
    pud_node_id right_2 = 7;

    void SetUp() override {
        ON_CALL(leaves, check_leaf(_)).WillByDefault([&](pud_node_id node) {
            return live_leaves.contains(node);
        });
        ON_CALL(children, get(_)).WillByDefault([&](pud_node_id node) -> const std::vector<pud_node_id>& {
            return sequences.at(node);
        });
        ON_CALL(descend, descend(_, _)).WillByDefault([&](descent_t, pud_node_id child) -> std::optional<descent_t> {
            if (blocked_nodes.contains(child))
                return std::nullopt;
            return descent_t{child};
        });
    }

    candidate_head_t make_head(pud_node_id node) {
        return candidate_head_t{
            witnesses, witnesses, witnesses, leaves, children, descend,
            descent_t{node}};
    }

    void add_node(pud_node_id node, std::vector<pud_node_id> node_children) {
        sequences[node] = std::move(node_children);
    }

    void add_leaf(pud_node_id node) {
        sequences[node] = {};
        live_leaves.insert(node);
    }

    // node 1 -> 2 -> ... -> bottom -> {bottom + 1, bottom + 2}; both bottom children are leaves.
    void add_chain_with_leaf_pair_at_bottom(pud_node_id bottom) {
        for (pud_node_id node = 1; node < bottom; ++node)
            add_node(node, {node + 1});
        add_node(bottom, {bottom + 1, bottom + 2});
        add_leaf(bottom + 1);
        add_leaf(bottom + 2);
    }

    // Returns the witness heads that lost their last leaf.
    std::vector<pud_mhws_head_id> kill_leaf(pud_node_id leaf) {
        live_leaves.erase(leaf);
        return witnesses.invalidate_leaf(leaf);
    }

    bool has_live_leaf(pud_node_id node) const {
        if (live_leaves.contains(node))
            return true;
        for (pud_node_id child : sequences.at(node)) {
            if (has_live_leaf(child))
                return true;
        }
        return false;
    }

    expected_outcome find_expected_outcome(pud_node_id start) const {
        pud_node_id node = start;
        while (true) {
            if (live_leaves.contains(node))
                return {outcome_kind::self_witness, node};
            std::vector<pud_node_id> live_children;
            for (pud_node_id child : sequences.at(node)) {
                if (!has_live_leaf(child))
                    continue;
                live_children.push_back(child);
            }
            const bool is_choice_point = live_children.size() >= 2;
            if (is_choice_point)
                return {outcome_kind::choice_point, node};
            const bool is_refuted = live_children.empty();
            if (is_refuted)
                return {outcome_kind::none, node};
            node = live_children.front();
        }
    }

    void expect_outcome(const optional_context_t& actual, const expected_outcome& expected) {
        if (expected.kind == outcome_kind::none) {
            EXPECT_FALSE(actual.has_value());
            return;
        }
        if (!actual.has_value()) {
            ADD_FAILURE() << "expected a justification at node " << expected.node;
            return;
        }
        EXPECT_EQ(actual->descent.node, expected.node);
        const auto* self = std::get_if<pud_candidate_self_witness>(&actual->justification);
        const auto* choice = std::get_if<pud_candidate_choice_point>(&actual->justification);
        if (expected.kind == outcome_kind::self_witness) {
            ASSERT_NE(self, nullptr) << "expected a self-witness at node " << expected.node;
            EXPECT_EQ(self->node, expected.node);
            return;
        }
        ASSERT_NE(choice, nullptr) << "expected a choice point at node " << expected.node;
        EXPECT_NE(choice->witness_a, choice->witness_b);
    }

    optional_context_t resume_tracked(tracked_head& tracked) {
        optional_context_t found = tracked.head.resume();
        const auto* choice = found.has_value()
            ? std::get_if<pud_candidate_choice_point>(&found->justification)
            : nullptr;
        tracked.owned.clear();
        tracked.finished = (choice == nullptr);
        if (choice != nullptr)
            tracked.owned = {choice->witness_a, choice->witness_b};
        return found;
    }

    tracked_head* find_owner(pud_mhws_head_id witness_id, const std::vector<tracked_head*>& heads) {
        for (tracked_head* tracked : heads) {
            if (std::ranges::find(tracked->owned, witness_id) != tracked->owned.end())
                return tracked;
        }
        return nullptr;
    }

    void deliver_lost(const std::vector<pud_mhws_head_id>& lost, const std::vector<tracked_head*>& heads) {
        for (pud_mhws_head_id witness_id : lost) {
            tracked_head* owner = find_owner(witness_id, heads);
            if (owner == nullptr) {
                ADD_FAILURE() << "witness " << witness_id << " was lost but no candidate head owns it";
                continue;
            }
            owner->head.witness_refuted(witness_id);
            std::erase(owner->owned, witness_id);
        }
    }

    // Builds a random tree rooted at node 1. Childless nodes are live leaves
    // most of the time and dead ends otherwise.
    void build_random_tree(std::mt19937& rng) {
        std::uniform_int_distribution<int> percent(0, 99);
        std::uniform_int_distribution<int> child_count_dist(1, 4);
        std::queue<std::pair<pud_node_id, uint32_t>> pending;
        pud_node_id next_id = 2;
        pending.push({root, 0});
        while (!pending.empty()) {
            auto [node, depth] = pending.front();
            pending.pop();
            const bool is_root = (node == root);
            const bool has_room = depth < k_random_tree_max_depth && next_id + 4 < k_random_tree_max_nodes;
            const bool expands = is_root || (has_room && percent(rng) < 70);
            if (!expands) {
                const bool is_live_leaf = percent(rng) < 85;
                if (is_live_leaf)
                    add_leaf(node);
                else
                    add_node(node, {});
                continue;
            }
            std::vector<pud_node_id> node_children;
            const int child_count = child_count_dist(rng);
            for (int i = 0; i < child_count; ++i) {
                node_children.push_back(next_id);
                pending.push({next_id, depth + 1});
                ++next_id;
            }
            add_node(node, node_children);
        }
    }

    std::vector<pud_node_id> shuffle_live_leaves(std::mt19937& rng) {
        std::vector<pud_node_id> order(live_leaves.begin(), live_leaves.end());
        std::ranges::sort(order);
        std::ranges::shuffle(order, rng);
        return order;
    }
};

// ── Small hand-built trees ───────────────────────────────────────────────────

TEST_F(PudCandidateSpecializationHeadSearchIntegrationTest, BinaryTreeStartsAsChoicePointAtRoot) {
    add_node(root, {left, right});
    add_node(left, {left_1, left_2});
    add_node(right, {right_1, right_2});
    for (pud_node_id leaf : {left_1, left_2, right_1, right_2})
        add_leaf(leaf);
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);
    EXPECT_NE(cp->witness_a, cp->witness_b);
    EXPECT_EQ(found->descent, descent_t{root});
}

// Refutations push the head one level down at a time until a single leaf is left.
TEST_F(PudCandidateSpecializationHeadSearchIntegrationTest, RefutedWitnessesDriveHeadDownToSelfWitness) {
    add_node(root, {left, right});
    add_node(left, {left_1, left_2});
    add_node(right, {right_1, right_2});
    for (pud_node_id leaf : {left_1, left_2, right_1, right_2})
        add_leaf(leaf);
    auto head = make_head(root);
    auto first = head.resume();
    ASSERT_TRUE(first.has_value());
    const auto* first_cp = std::get_if<pud_candidate_choice_point>(&first->justification);
    ASSERT_NE(first_cp, nullptr);

    // right's witness silently moves to right_2; nothing is refuted
    EXPECT_TRUE(kill_leaf(right_1).empty());

    // right has no leaves left: witness b is refuted, head descends into left
    auto lost_right = kill_leaf(right_2);
    ASSERT_EQ(lost_right.size(), 1u);
    EXPECT_EQ(lost_right[0], first_cp->witness_b);
    head.witness_refuted(lost_right[0]);
    auto second = head.resume();
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->descent, descent_t{left});
    const auto* second_cp = std::get_if<pud_candidate_choice_point>(&second->justification);
    ASSERT_NE(second_cp, nullptr);
    EXPECT_EQ(second_cp->witness_a, first_cp->witness_a);
    EXPECT_NE(second_cp->witness_b, first_cp->witness_b);

    // left_2 dies: only left_1 remains, so the head ends as a self-witness there
    auto lost_left = kill_leaf(left_2);
    ASSERT_EQ(lost_left.size(), 1u);
    EXPECT_EQ(lost_left[0], second_cp->witness_b);
    head.witness_refuted(lost_left[0]);
    auto third = head.resume();
    ASSERT_TRUE(third.has_value());
    const auto* self = std::get_if<pud_candidate_self_witness>(&third->justification);
    ASSERT_NE(self, nullptr);
    EXPECT_EQ(self->node, left_1);
    EXPECT_EQ(third->descent, descent_t{left_1});
}

TEST_F(PudCandidateSpecializationHeadSearchIntegrationTest, RefutedWitnessIsReplacedBySiblingSubtree) {
    pud_node_id third_child = 8;
    add_node(root, {left, right, third_child});
    add_leaf(left);
    add_leaf(right);
    add_leaf(third_child);
    auto head = make_head(root);
    auto first = head.resume();
    ASSERT_TRUE(first.has_value());
    const auto* first_cp = std::get_if<pud_candidate_choice_point>(&first->justification);
    ASSERT_NE(first_cp, nullptr);

    auto lost = kill_leaf(left);
    ASSERT_EQ(lost.size(), 1u);
    EXPECT_EQ(lost[0], first_cp->witness_a);
    head.witness_refuted(lost[0]);
    auto second = head.resume();
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->descent, descent_t{root});
    const auto* second_cp = std::get_if<pud_candidate_choice_point>(&second->justification);
    ASSERT_NE(second_cp, nullptr);
    EXPECT_NE(second_cp->witness_a, first_cp->witness_a);
    EXPECT_EQ(second_cp->witness_b, first_cp->witness_b);
}

// ── Deep and wide stress ─────────────────────────────────────────────────────

TEST_F(PudCandidateSpecializationHeadSearchIntegrationTest, DeepChainAdvancesToChoicePointAtTheBottom) {
    add_chain_with_leaf_pair_at_bottom(k_chain_depth);
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(found->descent, descent_t{k_chain_depth});
}

TEST_F(PudCandidateSpecializationHeadSearchIntegrationTest, WideRootWithOnlyTheLastTwoChildrenAliveFormsChoicePoint) {
    std::vector<pud_node_id> wide_children;
    for (pud_node_id child = 2; child < 2 + k_wide_child_count; ++child) {
        wide_children.push_back(child);
        const bool is_last_two = child >= 2 + k_wide_child_count - 2;
        if (is_last_two)
            add_leaf(child);
        else
            add_node(child, {});
    }
    add_node(root, wide_children);
    const pud_node_id second_last = 2 + k_wide_child_count - 2;
    const pud_node_id last        = 2 + k_wide_child_count - 1;
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);

    auto lost = kill_leaf(last);
    ASSERT_EQ(lost.size(), 1u);
    EXPECT_EQ(lost[0], cp->witness_b);
    head.witness_refuted(lost[0]);
    auto next = head.resume();
    ASSERT_TRUE(next.has_value());
    const auto* self = std::get_if<pud_candidate_self_witness>(&next->justification);
    ASSERT_NE(self, nullptr);
    EXPECT_EQ(self->node, second_last);
}

// All heads share the same two leaves, so one leaf death must reach every head.
TEST_F(PudCandidateSpecializationHeadSearchIntegrationTest, ManyHeadsSharingALeafAreAllNotified) {
    add_node(root, {left, right});
    add_leaf(left);
    add_leaf(right);
    std::deque<candidate_head_t> heads;
    std::vector<pud_mhws_head_id> left_witnesses;
    for (uint32_t i = 0; i < k_shared_head_count; ++i) {
        heads.push_back(make_head(root));
        auto found = heads.back().resume();
        ASSERT_TRUE(found.has_value());
        const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
        ASSERT_NE(cp, nullptr);
        left_witnesses.push_back(cp->witness_a);
    }

    auto lost = kill_leaf(left);
    EXPECT_EQ(lost.size(), k_shared_head_count);
    std::vector<pud_mhws_head_id> sorted_lost = lost;
    std::vector<pud_mhws_head_id> sorted_expected = left_witnesses;
    std::ranges::sort(sorted_lost);
    std::ranges::sort(sorted_expected);
    EXPECT_EQ(sorted_lost, sorted_expected);

    for (uint32_t i = 0; i < k_shared_head_count; ++i) {
        heads[i].witness_refuted(left_witnesses[i]);
        auto next = heads[i].resume();
        ASSERT_TRUE(next.has_value());
        const auto* self = std::get_if<pud_candidate_self_witness>(&next->justification);
        ASSERT_NE(self, nullptr);
        EXPECT_EQ(self->node, right);
    }
}

// ── Forking ──────────────────────────────────────────────────────────────────

TEST_F(PudCandidateSpecializationHeadSearchIntegrationTest, ForkOfDeepHeadReplaysWholePathAndStaysIndependent) {
    add_chain_with_leaf_pair_at_bottom(k_chain_depth);
    const pud_node_id bottom_left  = k_chain_depth + 1;
    const pud_node_id bottom_right = k_chain_depth + 2;
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);

    candidate_head_t forked{head, descent_t{root}};
    auto forked_found = forked.resume();
    ASSERT_TRUE(forked_found.has_value());
    const auto* forked_cp = std::get_if<pud_candidate_choice_point>(&forked_found->justification);
    ASSERT_NE(forked_cp, nullptr);
    EXPECT_EQ(forked_found->descent, descent_t{k_chain_depth});
    EXPECT_NE(forked_cp->witness_a, cp->witness_a);
    EXPECT_NE(forked_cp->witness_b, cp->witness_b);

    // one leaf is shared by both heads' witnesses; each head is refuted separately
    auto lost = kill_leaf(bottom_left);
    EXPECT_THAT(lost, UnorderedElementsAre(cp->witness_a, forked_cp->witness_a));
    head.witness_refuted(cp->witness_a);
    forked.witness_refuted(forked_cp->witness_a);
    for (candidate_head_t* candidate : {&head, &forked}) {
        auto next = candidate->resume();
        ASSERT_TRUE(next.has_value());
        const auto* self = std::get_if<pud_candidate_self_witness>(&next->justification);
        ASSERT_NE(self, nullptr);
        EXPECT_EQ(self->node, bottom_right);
    }
}

TEST_F(PudCandidateSpecializationHeadSearchIntegrationTest, ForkBlockedOnFirstPathStepYieldsNullopt) {
    add_chain_with_leaf_pair_at_bottom(3);
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    blocked_nodes.insert(2);
    candidate_head_t forked{head, descent_t{root}};
    EXPECT_FALSE(forked.resume().has_value());
}

TEST_F(PudCandidateSpecializationHeadSearchIntegrationTest, ForkBlockedMidPathYieldsNulloptAndLeaksNoWitnesses) {
    add_chain_with_leaf_pair_at_bottom(3);
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);

    blocked_nodes.insert(3);
    candidate_head_t forked{head, descent_t{root}};
    EXPECT_FALSE(forked.resume().has_value());

    // the original is untouched and the failed fork left no witness heads behind
    auto again = head.resume();
    ASSERT_TRUE(again.has_value());
    const auto* again_cp = std::get_if<pud_candidate_choice_point>(&again->justification);
    ASSERT_NE(again_cp, nullptr);
    EXPECT_EQ(again_cp->witness_a, cp->witness_a);
    EXPECT_EQ(again_cp->witness_b, cp->witness_b);
    EXPECT_THAT(kill_leaf(4), UnorderedElementsAre(cp->witness_a));
}

// The fork cannot follow either of the original's witnesses; it must search the
// siblings the original had not scanned yet.
TEST_F(PudCandidateSpecializationHeadSearchIntegrationTest, ForkWithBothWitnessRootsBlockedUsesUnscannedSiblings) {
    pud_node_id third_child  = 8;
    pud_node_id fourth_child = 9;
    add_node(root, {left, right, third_child, fourth_child});
    for (pud_node_id leaf : {left, right, third_child, fourth_child})
        add_leaf(leaf);
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);

    blocked_nodes.insert(left);
    blocked_nodes.insert(right);
    candidate_head_t forked{head, descent_t{root}};
    auto forked_found = forked.resume();
    ASSERT_TRUE(forked_found.has_value());
    const auto* forked_cp = std::get_if<pud_candidate_choice_point>(&forked_found->justification);
    ASSERT_NE(forked_cp, nullptr);
    EXPECT_EQ(forked_found->descent, descent_t{root});
    EXPECT_NE(forked_cp->witness_a, cp->witness_a);
    EXPECT_NE(forked_cp->witness_b, cp->witness_b);

    // the third child belongs only to the fork's witnesses
    auto lost = kill_leaf(third_child);
    ASSERT_EQ(lost.size(), 1u);
    EXPECT_EQ(lost[0], forked_cp->witness_a);
    forked.witness_refuted(lost[0]);
    auto next = forked.resume();
    ASSERT_TRUE(next.has_value());
    const auto* self = std::get_if<pud_candidate_self_witness>(&next->justification);
    ASSERT_NE(self, nullptr);
    EXPECT_EQ(self->node, fourth_child);

    // the original is unaffected
    auto original_again = head.resume();
    ASSERT_TRUE(original_again.has_value());
    EXPECT_NE(std::get_if<pud_candidate_choice_point>(&original_again->justification), nullptr);
}

// ── Randomized trees checked against the oracle ──────────────────────────────

struct PudCandidateSpecializationHeadRandomTreeIntegrationTest
    : public PudCandidateSpecializationHeadSearchIntegrationTest
    , public ::testing::WithParamInterface<uint32_t> {};

// Kill every leaf in random order; after each death the head must agree with a
// from-scratch computation of the shallowest choice point.
TEST_P(PudCandidateSpecializationHeadRandomTreeIntegrationTest, HeadMatchesOracleAsLeavesDie) {
    std::mt19937 rng(GetParam());
    build_random_tree(rng);
    const std::vector<pud_node_id> kill_order = shuffle_live_leaves(rng);
    tracked_head tracked{make_head(root), {}, false};
    expect_outcome(resume_tracked(tracked), find_expected_outcome(root));
    for (pud_node_id leaf : kill_order) {
        if (tracked.finished)
            break;
        deliver_lost(kill_leaf(leaf), {&tracked});
        expect_outcome(resume_tracked(tracked), find_expected_outcome(root));
    }
    EXPECT_TRUE(tracked.finished);
}

// Same, but a fork is taken part-way through; both heads must keep agreeing
// with the oracle while sharing the same witness store.
TEST_P(PudCandidateSpecializationHeadRandomTreeIntegrationTest, ForkAndOriginalBothMatchOracleAsLeavesDie) {
    std::mt19937 rng(GetParam());
    build_random_tree(rng);
    const std::vector<pud_node_id> kill_order = shuffle_live_leaves(rng);
    std::uniform_int_distribution<size_t> fork_point_dist(0, kill_order.size());
    const size_t fork_point = fork_point_dist(rng);

    tracked_head tracked{make_head(root), {}, false};
    std::optional<tracked_head> forked;
    expect_outcome(resume_tracked(tracked), find_expected_outcome(root));
    for (size_t step = 0; step < kill_order.size(); ++step) {
        const bool fork_now = (step == fork_point) && !forked.has_value() && !tracked.finished;
        if (fork_now) {
            forked.emplace(tracked_head{candidate_head_t{tracked.head, descent_t{root}}, {}, false});
            expect_outcome(resume_tracked(*forked), find_expected_outcome(root));
        }

        std::vector<tracked_head*> active;
        if (!tracked.finished)
            active.push_back(&tracked);
        if (forked.has_value() && !forked->finished)
            active.push_back(&*forked);
        if (active.empty())
            break;

        deliver_lost(kill_leaf(kill_order[step]), active);
        for (tracked_head* head : active)
            expect_outcome(resume_tracked(*head), find_expected_outcome(root));
    }
    EXPECT_TRUE(tracked.finished);
    if (forked.has_value())
        EXPECT_TRUE(forked->finished);
}

INSTANTIATE_TEST_SUITE_P(
    Seeds,
    PudCandidateSpecializationHeadRandomTreeIntegrationTest,
    ::testing::Range(0u, k_random_tree_count));
