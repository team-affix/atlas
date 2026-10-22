#include <gtest/gtest.h>
#include <algorithm>
#include <deque>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "infrastructure/structure_watcher.hpp"

namespace {

std::vector<watcher_head_id> sorted_heads(std::vector<watcher_head_id> v) {
    std::sort(v.begin(), v.end());
    return v;
}

std::vector<uint32_t> sorted_reps(std::vector<uint32_t> v) {
    std::sort(v.begin(), v.end());
    return v;
}

// Minimal get_parent backed by a user-managed map.
// Returns nullopt for unknown nodes (root nodes).
struct TestGetParent {
    std::unordered_map<pud_node_id, pud_node_id> parent_map;
    std::optional<pud_node_id> get(pud_node_id id) const {
        const auto it = parent_map.find(id);
        if (it == parent_map.end())
            return std::nullopt;
        return it->second;
    }
};

void check_mirror(const structure_watcher<TestGetParent>& sw,
                  pud_node_id node,
                  const std::vector<uint32_t>& all_reps,
                  const std::vector<watcher_head_id>& all_heads) {
    for (uint32_t rep : all_reps) {
        for (watcher_head_id head : sw.heads_of(node, rep)) {
            const auto reps = sorted_reps(sw.reps_of(node, head));
            EXPECT_TRUE(std::binary_search(reps.begin(), reps.end(), rep))
                << "mirror broken: rep " << rep << " has head " << head
                << " but head's reps don't include rep";
        }
    }
    for (watcher_head_id head : all_heads) {
        for (uint32_t rep : sw.reps_of(node, head)) {
            const auto heads = sorted_heads(sw.heads_of(node, rep));
            EXPECT_TRUE(std::binary_search(heads.begin(), heads.end(), head))
                << "mirror broken: head " << head << " has rep " << rep
                << " but rep's heads don't include head";
        }
    }
}

} // namespace

struct StructureWatcherTest : public ::testing::Test {
    pud_node_id next_id_ = 1;
    TestGetParent get_parent_;
    structure_watcher<TestGetParent> sw_{get_parent_};

    // Allocate a node with a given parent (0 = root, no parent).
    pud_node_id alloc(pud_node_id parent) {
        pud_node_id id = next_id_++;
        if (parent != 0)
            get_parent_.parent_map[id] = parent;
        return id;
    }
};

// ---------------------------------------------------------------------------
// Before any watch
// ---------------------------------------------------------------------------

TEST_F(StructureWatcherTest, EmptyWatcherHeadsOfAndRepsOfAreEmpty) {
    pud_node_id root = alloc(0);
    EXPECT_TRUE(sw_.heads_of(root, 0u).empty());
    EXPECT_TRUE(sw_.reps_of(root, 0u).empty());
}

TEST_F(StructureWatcherTest, NoteVarBindOnUnwatchedRepReturnsEmpty) {
    pud_node_id root = alloc(0);
    EXPECT_TRUE(sw_.note_var_bind(root, 1u, 2u).empty());
    EXPECT_TRUE(sw_.heads_of(root, 1u).empty());
    EXPECT_TRUE(sw_.heads_of(root, 2u).empty());
}

TEST_F(StructureWatcherTest, NoteFunctorBindOnUnwatchedRepReturnsEmpty) {
    pud_node_id root = alloc(0);
    EXPECT_TRUE(sw_.note_functor_bind(root, 1u, {2u, 3u}).empty());
}

// ---------------------------------------------------------------------------
// watch basics
// ---------------------------------------------------------------------------

TEST_F(StructureWatcherTest, WatchEmptyRepList) {
    pud_node_id root = alloc(0);
    sw_.watch(root, 0u, {});
    EXPECT_TRUE(sw_.reps_of(root, 0u).empty());
    EXPECT_TRUE(sw_.heads_of(root, 99u).empty());
}

TEST_F(StructureWatcherTest, WatchOneRep) {
    pud_node_id root = alloc(0);
    sw_.watch(root, 1u, {10u});
    EXPECT_EQ(sw_.reps_of(root, 1u), std::vector<uint32_t>{10u});
    EXPECT_EQ(sw_.heads_of(root, 10u), std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, WatchManyRepsIncludingZeroAndLargeId) {
    pud_node_id root = alloc(0);
    sw_.watch(root, 5u, {0u, 100u, 0xffffffffu});
    const auto reps = sw_.reps_of(root, 5u);
    EXPECT_EQ(reps, (std::vector<uint32_t>{0u, 100u, 0xffffffffu}));
    EXPECT_EQ(sw_.heads_of(root, 0u),          std::vector<watcher_head_id>{5u});
    EXPECT_EQ(sw_.heads_of(root, 100u),         std::vector<watcher_head_id>{5u});
    EXPECT_EQ(sw_.heads_of(root, 0xffffffffu),  std::vector<watcher_head_id>{5u});
}

TEST_F(StructureWatcherTest, DuplicateRepsInWatchCollapseToOne) {
    pud_node_id root = alloc(0);
    sw_.watch(root, 1u, {7u, 7u, 7u});
    EXPECT_EQ(sw_.reps_of(root, 1u), std::vector<uint32_t>{7u});
    EXPECT_EQ(sw_.heads_of(root, 7u), std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, TwoHeadsSameRep) {
    pud_node_id root = alloc(0);
    sw_.watch(root, 1u, {5u});
    sw_.watch(root, 2u, {5u});
    const auto heads = sorted_heads(sw_.heads_of(root, 5u));
    EXPECT_EQ(heads, (std::vector<watcher_head_id>{1u, 2u}));
    EXPECT_EQ(sw_.reps_of(root, 1u), std::vector<uint32_t>{5u});
    EXPECT_EQ(sw_.reps_of(root, 2u), std::vector<uint32_t>{5u});
}

TEST_F(StructureWatcherTest, TwoHeadsDisjointReps) {
    pud_node_id root = alloc(0);
    sw_.watch(root, 1u, {10u});
    sw_.watch(root, 2u, {20u});
    EXPECT_EQ(sw_.heads_of(root, 10u), std::vector<watcher_head_id>{1u});
    EXPECT_EQ(sw_.heads_of(root, 20u), std::vector<watcher_head_id>{2u});
    EXPECT_TRUE(sw_.heads_of(root, 20u) != std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, TwoHeadsOverlappingReps) {
    pud_node_id root = alloc(0);
    sw_.watch(root, 1u, {10u, 20u});
    sw_.watch(root, 2u, {20u, 30u});
    const auto h10 = sorted_heads(sw_.heads_of(root, 10u));
    const auto h20 = sorted_heads(sw_.heads_of(root, 20u));
    const auto h30 = sorted_heads(sw_.heads_of(root, 30u));
    EXPECT_EQ(h10, (std::vector<watcher_head_id>{1u}));
    EXPECT_EQ(h20, (std::vector<watcher_head_id>{1u, 2u}));
    EXPECT_EQ(h30, (std::vector<watcher_head_id>{2u}));
}

// ---------------------------------------------------------------------------
// Inheritance and isolation
// ---------------------------------------------------------------------------

TEST_F(StructureWatcherTest, ChildInheritsParentWatch) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    sw_.watch(root, 1u, {5u});
    EXPECT_EQ(sw_.heads_of(child, 5u), std::vector<watcher_head_id>{1u});
    EXPECT_EQ(sw_.reps_of(child, 1u),  std::vector<uint32_t>{5u});
}

TEST_F(StructureWatcherTest, SiblingDoesNotInheritOtherSiblingWatch) {
    pud_node_id root      = alloc(0);
    pud_node_id sibling_a = alloc(root);
    pud_node_id sibling_b = alloc(root);
    sw_.watch(sibling_a, 1u, {5u});
    EXPECT_TRUE(sw_.heads_of(sibling_b, 5u).empty());
    EXPECT_TRUE(sw_.reps_of(sibling_b, 1u).empty());
}

TEST_F(StructureWatcherTest, GrandchildInheritsChain) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    pud_node_id grand = alloc(child);
    sw_.watch(root, 1u, {5u});
    EXPECT_EQ(sw_.heads_of(grand, 5u), std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, HeadWatchedOnChildInvisibleToParentAndSibling) {
    pud_node_id root    = alloc(0);
    pud_node_id child   = alloc(root);
    pud_node_id sibling = alloc(root);
    sw_.watch(child, 1u, {5u});
    EXPECT_TRUE(sw_.heads_of(root,    5u).empty());
    EXPECT_TRUE(sw_.heads_of(sibling, 5u).empty());
    EXPECT_EQ(sw_.heads_of(child, 5u), std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, SeveralWatchesOnSameNodeAllVisible) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    sw_.watch(child, 1u, {5u});
    sw_.watch(child, 2u, {6u});
    sw_.watch(child, 3u, {5u, 6u});
    EXPECT_EQ(sorted_heads(sw_.heads_of(child, 5u)),
              (std::vector<watcher_head_id>{1u, 3u}));
    EXPECT_EQ(sorted_heads(sw_.heads_of(child, 6u)),
              (std::vector<watcher_head_id>{2u, 3u}));
    EXPECT_TRUE(sw_.heads_of(root, 5u).empty());
}

// ---------------------------------------------------------------------------
// note_var_bind
// ---------------------------------------------------------------------------

TEST_F(StructureWatcherTest, VarBindEveryHeadAlreadyContainsTarget) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    sw_.watch(root, 1u, {10u, 20u});
    sw_.watch(root, 2u, {10u, 20u});
    const auto changed = sorted_heads(sw_.note_var_bind(child, 10u, 20u));
    EXPECT_EQ(changed, (std::vector<watcher_head_id>{1u, 2u}));
    EXPECT_EQ(sw_.reps_of(child, 1u), std::vector<uint32_t>{20u});
    EXPECT_EQ(sw_.reps_of(child, 2u), std::vector<uint32_t>{20u});
    EXPECT_TRUE(sw_.heads_of(child, 10u).empty());
    const auto h20 = sorted_heads(sw_.heads_of(child, 20u));
    EXPECT_EQ(h20, (std::vector<watcher_head_id>{1u, 2u}));
    // parent unchanged
    EXPECT_EQ(sorted_reps(sw_.reps_of(root, 1u)), (std::vector<uint32_t>{10u, 20u}));
}

TEST_F(StructureWatcherTest, VarBindNoHeadContainsTarget) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    sw_.watch(root, 1u, {10u});
    sw_.watch(root, 2u, {10u});
    const auto changed = sw_.note_var_bind(child, 10u, 30u);
    EXPECT_TRUE(changed.empty());
    EXPECT_EQ(sw_.reps_of(child, 1u), std::vector<uint32_t>{30u});
    EXPECT_EQ(sw_.reps_of(child, 2u), std::vector<uint32_t>{30u});
    EXPECT_TRUE(sw_.heads_of(child, 10u).empty());
    const auto h30 = sorted_heads(sw_.heads_of(child, 30u));
    EXPECT_EQ(h30, (std::vector<watcher_head_id>{1u, 2u}));
}

TEST_F(StructureWatcherTest, VarBindMixedHeads) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    const uint32_t bound  = 10u;
    const uint32_t target = 20u;
    sw_.watch(root, 1u, {bound, target});
    sw_.watch(root, 2u, {bound});
    sw_.watch(root, 3u, {target});
    const auto changed = sorted_heads(sw_.note_var_bind(child, bound, target));
    EXPECT_EQ(changed, std::vector<watcher_head_id>{1u});
    EXPECT_EQ(sw_.reps_of(child, 1u), std::vector<uint32_t>{target});
    EXPECT_EQ(sw_.reps_of(child, 2u), std::vector<uint32_t>{target});
    EXPECT_EQ(sw_.reps_of(child, 3u), std::vector<uint32_t>{target});
    EXPECT_TRUE(sw_.heads_of(child, bound).empty());
    const auto h_target = sorted_heads(sw_.heads_of(child, target));
    EXPECT_EQ(h_target, (std::vector<watcher_head_id>{1u, 2u, 3u}));
}

TEST_F(StructureWatcherTest, FrontierMoveOntoRepAlreadyWatchedByOtherHead) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    sw_.watch(root, 1u, {10u});
    sw_.watch(root, 2u, {20u});
    const auto changed = sw_.note_var_bind(child, 10u, 20u);
    EXPECT_TRUE(changed.empty());
    const auto h20 = sorted_heads(sw_.heads_of(child, 20u));
    EXPECT_EQ(h20, (std::vector<watcher_head_id>{1u, 2u}));
    EXPECT_EQ(sw_.reps_of(child, 2u), std::vector<uint32_t>{20u});
}

TEST_F(StructureWatcherTest, CollapseTargetIsOnlyOtherRepHeadBecomesMonotone) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    sw_.watch(root, 1u, {10u, 20u});
    const auto changed = sorted_heads(sw_.note_var_bind(child, 10u, 20u));
    EXPECT_EQ(changed, std::vector<watcher_head_id>{1u});
    EXPECT_EQ(sw_.reps_of(child, 1u), std::vector<uint32_t>{20u});
}

TEST_F(StructureWatcherTest, CollapseHeadHasOtherRepsTheyStay) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    sw_.watch(root, 1u, {10u, 20u, 30u});
    const auto changed = sorted_heads(sw_.note_var_bind(child, 10u, 20u));
    EXPECT_EQ(changed, std::vector<watcher_head_id>{1u});
    const auto reps = sorted_reps(sw_.reps_of(child, 1u));
    EXPECT_EQ(reps, (std::vector<uint32_t>{20u, 30u}));
}

TEST_F(StructureWatcherTest, TwoVarBindsSameNodeSecondSeesFirst) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    sw_.watch(root, 1u, {10u, 30u});
    const auto changed1 = sorted_heads(sw_.note_var_bind(child, 10u, 20u));
    EXPECT_TRUE(changed1.empty());
    const auto changed2 = sorted_heads(sw_.note_var_bind(child, 20u, 30u));
    EXPECT_EQ(changed2, std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, ChainOfFrontierMovesDownSpine) {
    pud_node_id root = alloc(0);
    sw_.watch(root, 1u, {0u});
    std::vector<pud_node_id> levels{root};
    for (int i = 0; i < 10; ++i)
        levels.push_back(alloc(levels.back()));

    for (int i = 1; i <= 9; i += 2) {
        const uint32_t from = static_cast<uint32_t>((i - 1) / 2);
        sw_.note_var_bind(levels[i], from, from + 1u);
    }

    for (int i = 1; i <= 9; ++i) {
        const uint32_t expected_rep = static_cast<uint32_t>((i + 1) / 2);
        const auto reps = sw_.reps_of(levels[i], 1u);
        ASSERT_EQ(reps.size(), 1u) << "level " << i;
        EXPECT_EQ(reps[0], expected_rep) << "level " << i;
    }
    EXPECT_EQ(sw_.reps_of(root, 1u), std::vector<uint32_t>{0u});
}

TEST_F(StructureWatcherTest, ParentUnchangedAfterChildBind) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    sw_.watch(root, 1u, {10u, 20u});
    sw_.note_var_bind(child, 10u, 20u);
    const auto parent_reps = sorted_reps(sw_.reps_of(root, 1u));
    EXPECT_EQ(parent_reps, (std::vector<uint32_t>{10u, 20u}));
    const auto parent_heads_10 = sw_.heads_of(root, 10u);
    EXPECT_EQ(parent_heads_10, std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, SiblingUnaffectedByOtherSiblingBind) {
    pud_node_id root  = alloc(0);
    pud_node_id sib_a = alloc(root);
    pud_node_id sib_b = alloc(root);
    sw_.watch(root, 1u, {10u, 20u});
    sw_.note_var_bind(sib_a, 10u, 20u);
    const auto b_reps = sorted_reps(sw_.reps_of(sib_b, 1u));
    EXPECT_EQ(b_reps, (std::vector<uint32_t>{10u, 20u}));
}

// ---------------------------------------------------------------------------
// note_functor_bind
// ---------------------------------------------------------------------------

TEST_F(StructureWatcherTest, FunctorBindNoIntroducedReps) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    sw_.watch(root, 1u, {5u});
    sw_.watch(root, 2u, {5u, 6u});
    const auto changed = sorted_heads(sw_.note_functor_bind(child, 5u, {}));
    EXPECT_EQ(changed, (std::vector<watcher_head_id>{1u, 2u}));
    EXPECT_TRUE(sw_.reps_of(child, 1u).empty());
    const auto r2 = sw_.reps_of(child, 2u);
    EXPECT_EQ(r2, std::vector<uint32_t>{6u});
    EXPECT_TRUE(sw_.heads_of(child, 5u).empty());
}

TEST_F(StructureWatcherTest, FunctorBindAllIntroducedRepsAlreadyInHead) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    sw_.watch(root, 1u, {5u, 10u, 20u});
    const auto changed = sorted_heads(sw_.note_functor_bind(child, 5u, {10u, 20u}));
    EXPECT_EQ(changed, std::vector<watcher_head_id>{1u});
    const auto reps = sorted_reps(sw_.reps_of(child, 1u));
    EXPECT_EQ(reps, (std::vector<uint32_t>{10u, 20u}));
}

TEST_F(StructureWatcherTest, FunctorBindAllIntroducedRepsNew) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    sw_.watch(root, 1u, {5u});
    const auto changed = sorted_heads(sw_.note_functor_bind(child, 5u, {10u, 20u}));
    EXPECT_EQ(changed, std::vector<watcher_head_id>{1u});
    const auto reps = sorted_reps(sw_.reps_of(child, 1u));
    EXPECT_EQ(reps, (std::vector<uint32_t>{10u, 20u}));
    EXPECT_EQ(sw_.heads_of(child, 10u), std::vector<watcher_head_id>{1u});
    EXPECT_EQ(sw_.heads_of(child, 20u), std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, FunctorBindOnlySomeHeadsWatchedBound) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    sw_.watch(root, 1u, {5u});
    sw_.watch(root, 2u, {6u});
    const auto changed = sorted_heads(sw_.note_functor_bind(child, 5u, {10u}));
    EXPECT_EQ(changed, std::vector<watcher_head_id>{1u});
    EXPECT_EQ(sw_.reps_of(child, 2u), std::vector<uint32_t>{6u});
}

TEST_F(StructureWatcherTest, FunctorBindThenChildVarBind) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    pud_node_id grand = alloc(child);
    sw_.watch(root, 1u, {5u});
    sw_.note_functor_bind(child, 5u, {10u, 20u});
    EXPECT_EQ(sorted_reps(sw_.reps_of(child, 1u)),
              (std::vector<uint32_t>{10u, 20u}));
    const auto changed = sorted_heads(sw_.note_var_bind(grand, 10u, 20u));
    EXPECT_EQ(changed, std::vector<watcher_head_id>{1u});
    EXPECT_EQ(sw_.reps_of(grand, 1u), std::vector<uint32_t>{20u});
    EXPECT_EQ(sorted_reps(sw_.reps_of(child, 1u)),
              (std::vector<uint32_t>{10u, 20u}));
}

TEST_F(StructureWatcherTest, FunctorBindExistingRepForOtherHeadNoDuplicate) {
    pud_node_id root  = alloc(0);
    pud_node_id child = alloc(root);
    sw_.watch(root, 1u, {5u});
    sw_.watch(root, 2u, {10u});
    sw_.note_functor_bind(child, 5u, {10u});
    EXPECT_EQ(sw_.reps_of(child, 2u), std::vector<uint32_t>{10u});
    const auto h10 = sorted_heads(sw_.heads_of(child, 10u));
    EXPECT_EQ(h10, (std::vector<watcher_head_id>{1u, 2u}));
}

// ---------------------------------------------------------------------------
// Ten-level spine and bush
// ---------------------------------------------------------------------------

TEST_F(StructureWatcherTest, TenLevelSpineRepsAtEachLevel) {
    pud_node_id root = alloc(0);
    sw_.watch(root, 1u, {0u});

    std::vector<pud_node_id> levels{root};
    for (int i = 0; i < 10; ++i)
        levels.push_back(alloc(levels.back()));

    for (int i = 1; i <= 9; i += 2) {
        const uint32_t from = static_cast<uint32_t>((i - 1) / 2);
        sw_.note_var_bind(levels[i], from, from + 1u);
    }

    for (int i = 1; i <= 9; ++i) {
        const uint32_t expected_rep = static_cast<uint32_t>((i + 1) / 2);
        const auto reps = sw_.reps_of(levels[i], 1u);
        ASSERT_EQ(reps.size(), 1u) << "level " << i;
        EXPECT_EQ(reps[0], expected_rep) << "level " << i;
    }
    EXPECT_EQ(sw_.reps_of(root, 1u), std::vector<uint32_t>{0u});
}

TEST_F(StructureWatcherTest, BushOfSiblingsSeeOwnBindOnly) {
    pud_node_id root = alloc(0);
    sw_.watch(root, 1u, {10u, 20u});
    sw_.watch(root, 2u, {30u, 40u});

    std::vector<pud_node_id> siblings;
    for (int i = 0; i < 5; ++i)
        siblings.push_back(alloc(root));

    sw_.note_var_bind(siblings[0], 10u, 20u);
    sw_.note_var_bind(siblings[1], 30u, 40u);

    EXPECT_EQ(sw_.reps_of(siblings[0], 1u), std::vector<uint32_t>{20u});
    EXPECT_EQ(sorted_reps(sw_.reps_of(siblings[0], 2u)),
              (std::vector<uint32_t>{30u, 40u}));

    EXPECT_EQ(sw_.reps_of(siblings[1], 2u), std::vector<uint32_t>{40u});
    EXPECT_EQ(sorted_reps(sw_.reps_of(siblings[1], 1u)),
              (std::vector<uint32_t>{10u, 20u}));

    EXPECT_EQ(sorted_reps(sw_.reps_of(siblings[2], 1u)),
              (std::vector<uint32_t>{10u, 20u}));
    EXPECT_EQ(sorted_reps(sw_.reps_of(siblings[2], 2u)),
              (std::vector<uint32_t>{30u, 40u}));
}

// ---------------------------------------------------------------------------
// Stress test: shadow map vs structure_watcher
// ---------------------------------------------------------------------------

namespace {

struct shadow_state {
    std::map<watcher_head_id, std::set<uint32_t>> head_to_reps;
    std::map<uint32_t, std::set<watcher_head_id>> rep_to_heads;

    void watch(watcher_head_id head, const std::vector<uint32_t>& reps) {
        for (uint32_t rep : reps) {
            head_to_reps[head].insert(rep);
            rep_to_heads[rep].insert(head);
        }
    }

    std::vector<watcher_head_id> note_var_bind(uint32_t bound, uint32_t target) {
        auto it = rep_to_heads.find(bound);
        if (it == rep_to_heads.end())
            return {};
        const std::set<watcher_head_id> watching = it->second;
        std::vector<watcher_head_id> changed;
        for (watcher_head_id head : watching) {
            auto& head_reps = head_to_reps[head];
            const bool collapse = head_reps.count(target) > 0;
            head_reps.erase(bound);
            rep_to_heads[bound].erase(head);
            if (!collapse) {
                head_reps.insert(target);
                rep_to_heads[target].insert(head);
            }
            if (collapse)
                changed.push_back(head);
        }
        if (rep_to_heads[bound].empty())
            rep_to_heads.erase(bound);
        std::sort(changed.begin(), changed.end());
        return changed;
    }

    std::vector<watcher_head_id> note_functor_bind(uint32_t bound,
                                                   const std::vector<uint32_t>& introduced) {
        auto it = rep_to_heads.find(bound);
        if (it == rep_to_heads.end())
            return {};
        const std::set<watcher_head_id> watching = it->second;
        std::vector<watcher_head_id> changed(watching.begin(), watching.end());
        rep_to_heads.erase(bound);
        for (watcher_head_id head : watching) {
            auto& head_reps = head_to_reps[head];
            head_reps.erase(bound);
            for (uint32_t rep : introduced) {
                head_reps.insert(rep);
                rep_to_heads[rep].insert(head);
            }
        }
        std::sort(changed.begin(), changed.end());
        return changed;
    }

    std::vector<watcher_head_id> heads_of(uint32_t rep) const {
        auto it = rep_to_heads.find(rep);
        if (it == rep_to_heads.end())
            return {};
        return std::vector<watcher_head_id>(it->second.begin(), it->second.end());
    }

    std::vector<uint32_t> reps_of(watcher_head_id head) const {
        auto it = head_to_reps.find(head);
        if (it == head_to_reps.end())
            return {};
        return std::vector<uint32_t>(it->second.begin(), it->second.end());
    }
};

} // namespace

TEST_F(StructureWatcherTest, StressRandomOpsAgainstShadow) {
    constexpr int k_ops        = 2000;
    constexpr int k_rep_pool   = 50;
    constexpr int k_head_pool  = 20;
    constexpr uint32_t k_seed  = 777u;

    std::mt19937 rng(k_seed);

    pud_node_id iv = alloc(0);
    shadow_state shadow;

    std::uniform_int_distribution<uint32_t> pick_rep(0u, static_cast<uint32_t>(k_rep_pool - 1));
    std::uniform_int_distribution<uint32_t> pick_head(0u, static_cast<uint32_t>(k_head_pool - 1));
    std::uniform_int_distribution<int>      pick_op(0, 2);
    std::uniform_int_distribution<int>      pick_reps_count(0, 8);
    std::uniform_int_distribution<int>      pick_intro_count(0, 6);

    auto verify = [&](int step) {
        for (int r = 0; r < k_rep_pool; ++r) {
            const auto sw_heads = sorted_heads(sw_.heads_of(iv, static_cast<uint32_t>(r)));
            const auto sh_heads = shadow.heads_of(static_cast<uint32_t>(r));
            ASSERT_EQ(sw_heads, sh_heads)
                << "step " << step << " rep " << r << " heads mismatch";
        }
        for (int h = 0; h < k_head_pool; ++h) {
            const auto sw_reps = sorted_reps(sw_.reps_of(iv, static_cast<watcher_head_id>(h)));
            const auto sh_reps = shadow.reps_of(static_cast<watcher_head_id>(h));
            ASSERT_EQ(sw_reps, sh_reps)
                << "step " << step << " head " << h << " reps mismatch";
        }
    };

    for (int step = 0; step < k_ops; ++step) {
        const int op = pick_op(rng);

        if (op == 0) {
            const watcher_head_id head = pick_head(rng);
            const int rep_count = pick_reps_count(rng);
            std::vector<uint32_t> reps;
            for (int j = 0; j < rep_count; ++j)
                reps.push_back(pick_rep(rng));
            shadow.watch(head, reps);
            sw_.watch(iv, head, reps);
        } else if (op == 1) {
            const uint32_t bound  = pick_rep(rng);
            const uint32_t target = pick_rep(rng);
            if (bound == target) continue;
            const auto sh_changed = shadow.note_var_bind(bound, target);
            const auto sw_changed = sorted_heads(sw_.note_var_bind(iv, bound, target));
            ASSERT_EQ(sw_changed, sh_changed)
                << "step " << step << " var bind " << bound << "->" << target;
        } else {
            const uint32_t bound = pick_rep(rng);
            const int intro_count = pick_intro_count(rng);
            std::vector<uint32_t> introduced;
            for (int j = 0; j < intro_count; ++j)
                introduced.push_back(pick_rep(rng));
            const auto sh_changed = shadow.note_functor_bind(bound, introduced);
            const auto sw_changed = sorted_heads(sw_.note_functor_bind(iv, bound, introduced));
            ASSERT_EQ(sw_changed, sh_changed)
                << "step " << step << " functor bind " << bound;
        }

        verify(step);
    }
}
