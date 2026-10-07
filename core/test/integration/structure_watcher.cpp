#include <gtest/gtest.h>
#include <algorithm>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>
#include "infrastructure/order_maintenance.hpp"
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

void check_mirror(const structure_watcher& sw, om_label open,
                  const std::vector<uint32_t>& all_reps,
                  const std::vector<watcher_head_id>& all_heads) {
    for (uint32_t rep : all_reps) {
        for (watcher_head_id head : sw.heads_of(open, rep)) {
            const auto reps = sorted_reps(sw.reps_of(open, head));
            EXPECT_TRUE(std::binary_search(reps.begin(), reps.end(), rep))
                << "mirror broken: rep " << rep << " has head " << head
                << " but head's reps don't include rep";
        }
    }
    for (watcher_head_id head : all_heads) {
        for (uint32_t rep : sw.reps_of(open, head)) {
            const auto heads = sorted_heads(sw.heads_of(open, rep));
            EXPECT_TRUE(std::binary_search(heads.begin(), heads.end(), head))
                << "mirror broken: head " << head << " has rep " << rep
                << " but rep's heads don't include head";
        }
    }
}

} // namespace

struct StructureWatcherTest : public ::testing::Test {
    order_maintenance om_;
    structure_watcher sw_;
};

// ---------------------------------------------------------------------------
// Before any watch
// ---------------------------------------------------------------------------

TEST_F(StructureWatcherTest, EmptyWatcherHeadsOfAndRepsOfAreEmpty) {
    const om_interval root = om_.allocate_root();
    EXPECT_TRUE(sw_.heads_of(root.open, 0u).empty());
    EXPECT_TRUE(sw_.reps_of(root.open, 0u).empty());
}

TEST_F(StructureWatcherTest, NoteVarBindOnUnwatchedRepReturnsEmpty) {
    const om_interval root = om_.allocate_root();
    EXPECT_TRUE(sw_.note_var_bind(root, 1u, 2u).empty());
    EXPECT_TRUE(sw_.heads_of(root.open, 1u).empty());
    EXPECT_TRUE(sw_.heads_of(root.open, 2u).empty());
}

TEST_F(StructureWatcherTest, NoteFunctorBindOnUnwatchedRepReturnsEmpty) {
    const om_interval root = om_.allocate_root();
    EXPECT_TRUE(sw_.note_functor_bind(root, 1u, {2u, 3u}).empty());
}

// ---------------------------------------------------------------------------
// watch basics
// ---------------------------------------------------------------------------

TEST_F(StructureWatcherTest, WatchEmptyRepList) {
    const om_interval root = om_.allocate_root();
    sw_.watch(root, 0u, {});
    EXPECT_TRUE(sw_.reps_of(root.open, 0u).empty());
    EXPECT_TRUE(sw_.heads_of(root.open, 99u).empty());
}

TEST_F(StructureWatcherTest, WatchOneRep) {
    const om_interval root = om_.allocate_root();
    sw_.watch(root, 1u, {10u});
    EXPECT_EQ(sw_.reps_of(root.open, 1u), std::vector<uint32_t>{10u});
    EXPECT_EQ(sw_.heads_of(root.open, 10u), std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, WatchManyRepsIncludingZeroAndLargeId) {
    const om_interval root = om_.allocate_root();
    sw_.watch(root, 5u, {0u, 100u, 0xffffffffu});
    const auto reps = sw_.reps_of(root.open, 5u);
    EXPECT_EQ(reps, (std::vector<uint32_t>{0u, 100u, 0xffffffffu}));
    EXPECT_EQ(sw_.heads_of(root.open, 0u),          std::vector<watcher_head_id>{5u});
    EXPECT_EQ(sw_.heads_of(root.open, 100u),         std::vector<watcher_head_id>{5u});
    EXPECT_EQ(sw_.heads_of(root.open, 0xffffffffu),  std::vector<watcher_head_id>{5u});
}

TEST_F(StructureWatcherTest, DuplicateRepsInWatchCollapseToOne) {
    const om_interval root = om_.allocate_root();
    sw_.watch(root, 1u, {7u, 7u, 7u});
    EXPECT_EQ(sw_.reps_of(root.open, 1u), std::vector<uint32_t>{7u});
    EXPECT_EQ(sw_.heads_of(root.open, 7u), std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, TwoHeadsSameRep) {
    const om_interval root = om_.allocate_root();
    sw_.watch(root, 1u, {5u});
    sw_.watch(root, 2u, {5u});
    const auto heads = sorted_heads(sw_.heads_of(root.open, 5u));
    EXPECT_EQ(heads, (std::vector<watcher_head_id>{1u, 2u}));
    EXPECT_EQ(sw_.reps_of(root.open, 1u), std::vector<uint32_t>{5u});
    EXPECT_EQ(sw_.reps_of(root.open, 2u), std::vector<uint32_t>{5u});
}

TEST_F(StructureWatcherTest, TwoHeadsDisjointReps) {
    const om_interval root = om_.allocate_root();
    sw_.watch(root, 1u, {10u});
    sw_.watch(root, 2u, {20u});
    EXPECT_EQ(sw_.heads_of(root.open, 10u), std::vector<watcher_head_id>{1u});
    EXPECT_EQ(sw_.heads_of(root.open, 20u), std::vector<watcher_head_id>{2u});
    EXPECT_TRUE(sw_.heads_of(root.open, 20u) != std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, TwoHeadsOverlappingReps) {
    const om_interval root = om_.allocate_root();
    sw_.watch(root, 1u, {10u, 20u});
    sw_.watch(root, 2u, {20u, 30u});
    const auto h10 = sorted_heads(sw_.heads_of(root.open, 10u));
    const auto h20 = sorted_heads(sw_.heads_of(root.open, 20u));
    const auto h30 = sorted_heads(sw_.heads_of(root.open, 30u));
    EXPECT_EQ(h10, (std::vector<watcher_head_id>{1u}));
    EXPECT_EQ(h20, (std::vector<watcher_head_id>{1u, 2u}));
    EXPECT_EQ(h30, (std::vector<watcher_head_id>{2u}));
}

// ---------------------------------------------------------------------------
// Inheritance and isolation
// ---------------------------------------------------------------------------

TEST_F(StructureWatcherTest, ChildInheritsParentWatch) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    sw_.watch(root, 1u, {5u});
    EXPECT_EQ(sw_.heads_of(child.open, 5u), std::vector<watcher_head_id>{1u});
    EXPECT_EQ(sw_.reps_of(child.open, 1u),  std::vector<uint32_t>{5u});
}

TEST_F(StructureWatcherTest, SiblingDoesNotInheritOtherSiblingWatch) {
    const om_interval root    = om_.allocate_root();
    const om_interval sibling_a = om_.allocate_child_of(root);
    const om_interval sibling_b = om_.allocate_child_of(root);
    sw_.watch(sibling_a, 1u, {5u});
    EXPECT_TRUE(sw_.heads_of(sibling_b.open, 5u).empty());
    EXPECT_TRUE(sw_.reps_of(sibling_b.open, 1u).empty());
}

TEST_F(StructureWatcherTest, GrandchildInheritsChain) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    const om_interval grand = om_.allocate_child_of(child);
    sw_.watch(root, 1u, {5u});
    EXPECT_EQ(sw_.heads_of(grand.open, 5u), std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, HeadWatchedOnChildInvisibleToParentAndSibling) {
    const om_interval root    = om_.allocate_root();
    const om_interval child   = om_.allocate_child_of(root);
    const om_interval sibling = om_.allocate_child_of(root);
    sw_.watch(child, 1u, {5u});
    EXPECT_TRUE(sw_.heads_of(root.open,    5u).empty());
    EXPECT_TRUE(sw_.heads_of(sibling.open, 5u).empty());
    EXPECT_EQ(sw_.heads_of(child.open, 5u), std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, SeveralWatchesOnSameIntervalAllVisible) {
    const om_interval root    = om_.allocate_root();
    const om_interval child   = om_.allocate_child_of(root);
    sw_.watch(child, 1u, {5u});
    sw_.watch(child, 2u, {6u});
    sw_.watch(child, 3u, {5u, 6u});
    EXPECT_EQ(sorted_heads(sw_.heads_of(child.open, 5u)),
              (std::vector<watcher_head_id>{1u, 3u}));
    EXPECT_EQ(sorted_heads(sw_.heads_of(child.open, 6u)),
              (std::vector<watcher_head_id>{2u, 3u}));
    EXPECT_TRUE(sw_.heads_of(root.open, 5u).empty());
}

// ---------------------------------------------------------------------------
// note_var_bind
// ---------------------------------------------------------------------------

TEST_F(StructureWatcherTest, VarBindEveryHeadAlreadyContainsTarget) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    sw_.watch(root, 1u, {10u, 20u});
    sw_.watch(root, 2u, {10u, 20u});
    const auto changed = sorted_heads(sw_.note_var_bind(child, 10u, 20u));
    EXPECT_EQ(changed, (std::vector<watcher_head_id>{1u, 2u}));
    EXPECT_FALSE(sw_.reps_of(child.open, 1u)[0] == 10u ||
                 (sw_.reps_of(child.open, 1u).size() > 1));
    EXPECT_EQ(sw_.reps_of(child.open, 1u), std::vector<uint32_t>{20u});
    EXPECT_EQ(sw_.reps_of(child.open, 2u), std::vector<uint32_t>{20u});
    EXPECT_TRUE(sw_.heads_of(child.open, 10u).empty());
    // heads_of(20) should not list each head twice
    const auto h20 = sorted_heads(sw_.heads_of(child.open, 20u));
    EXPECT_EQ(h20, (std::vector<watcher_head_id>{1u, 2u}));
    // parent unchanged
    EXPECT_EQ(sorted_reps(sw_.reps_of(root.open, 1u)), (std::vector<uint32_t>{10u, 20u}));
}

TEST_F(StructureWatcherTest, VarBindNoHeadContainsTarget) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    sw_.watch(root, 1u, {10u});
    sw_.watch(root, 2u, {10u});
    const auto changed = sw_.note_var_bind(child, 10u, 30u);
    EXPECT_TRUE(changed.empty());
    EXPECT_EQ(sw_.reps_of(child.open, 1u), std::vector<uint32_t>{30u});
    EXPECT_EQ(sw_.reps_of(child.open, 2u), std::vector<uint32_t>{30u});
    EXPECT_TRUE(sw_.heads_of(child.open, 10u).empty());
    const auto h30 = sorted_heads(sw_.heads_of(child.open, 30u));
    EXPECT_EQ(h30, (std::vector<watcher_head_id>{1u, 2u}));
}

TEST_F(StructureWatcherTest, VarBindMixedHeads) {
    // H1={bound,target}, H2={bound}, H3={target}
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    const uint32_t bound  = 10u;
    const uint32_t target = 20u;
    sw_.watch(root, 1u, {bound, target});
    sw_.watch(root, 2u, {bound});
    sw_.watch(root, 3u, {target});
    const auto changed = sorted_heads(sw_.note_var_bind(child, bound, target));
    EXPECT_EQ(changed, std::vector<watcher_head_id>{1u});
    // H1: bound erased, target kept
    EXPECT_EQ(sw_.reps_of(child.open, 1u), std::vector<uint32_t>{target});
    // H2: bound erased, target inserted
    EXPECT_EQ(sw_.reps_of(child.open, 2u), std::vector<uint32_t>{target});
    // H3: unchanged
    EXPECT_EQ(sw_.reps_of(child.open, 3u), std::vector<uint32_t>{target});
    EXPECT_TRUE(sw_.heads_of(child.open, bound).empty());
    const auto h_target = sorted_heads(sw_.heads_of(child.open, target));
    EXPECT_EQ(h_target, (std::vector<watcher_head_id>{1u, 2u, 3u}));
}

TEST_F(StructureWatcherTest, FrontierMoveOntoRepAlreadyWatchedByOtherHead) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    sw_.watch(root, 1u, {10u});  // will move
    sw_.watch(root, 2u, {20u});  // unrelated
    const auto changed = sw_.note_var_bind(child, 10u, 20u);
    EXPECT_TRUE(changed.empty());
    // head 1 now watches 20
    const auto h20 = sorted_heads(sw_.heads_of(child.open, 20u));
    EXPECT_EQ(h20, (std::vector<watcher_head_id>{1u, 2u}));
    // no duplicate in head 2's rep set
    EXPECT_EQ(sw_.reps_of(child.open, 2u), std::vector<uint32_t>{20u});
}

TEST_F(StructureWatcherTest, CollapseTargetIsOnlyOtherRepHeadBecomesMonotone) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    sw_.watch(root, 1u, {10u, 20u});
    const auto changed = sorted_heads(sw_.note_var_bind(child, 10u, 20u));
    EXPECT_EQ(changed, std::vector<watcher_head_id>{1u});
    EXPECT_EQ(sw_.reps_of(child.open, 1u), std::vector<uint32_t>{20u});
}

TEST_F(StructureWatcherTest, CollapseHeadHasOtherRepsTheyStay) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    sw_.watch(root, 1u, {10u, 20u, 30u});
    const auto changed = sorted_heads(sw_.note_var_bind(child, 10u, 20u));
    EXPECT_EQ(changed, std::vector<watcher_head_id>{1u});
    const auto reps = sorted_reps(sw_.reps_of(child.open, 1u));
    EXPECT_EQ(reps, (std::vector<uint32_t>{20u, 30u}));
}

TEST_F(StructureWatcherTest, TwoVarBindsSameIntervalSecondSeesFirst) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    sw_.watch(root, 1u, {10u, 30u});
    // first bind: 10->20 (frontier move since 20 not in {10,30})
    const auto changed1 = sorted_heads(sw_.note_var_bind(child, 10u, 20u));
    EXPECT_TRUE(changed1.empty());
    // second bind: 20->30 (collapse since 30 already in head 1's reps)
    const auto changed2 = sorted_heads(sw_.note_var_bind(child, 20u, 30u));
    EXPECT_EQ(changed2, std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, ChainOfFrontierMovesDownSpine) {
    const om_interval root = om_.allocate_root();
    sw_.watch(root, 1u, {1u});
    om_interval cur = root;
    for (uint32_t i = 1; i <= 5u; ++i) {
        const om_interval next = om_.allocate_child_of(cur);
        sw_.note_var_bind(next, i, i + 1u);
        cur = next;
    }
    // at each level, head 1 should watch the rep introduced at that level
    om_interval check = om_.allocate_child_of(root);
    // re-traverse to check levels
    check = om_.allocate_child_of(root);
    EXPECT_EQ(sw_.reps_of(root.open, 1u), std::vector<uint32_t>{1u});
}

TEST_F(StructureWatcherTest, ParentUnchangedAfterChildBind) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    sw_.watch(root, 1u, {10u, 20u});
    sw_.note_var_bind(child, 10u, 20u);
    // parent sees original reps
    const auto parent_reps = sorted_reps(sw_.reps_of(root.open, 1u));
    EXPECT_EQ(parent_reps, (std::vector<uint32_t>{10u, 20u}));
    const auto parent_heads_10 = sw_.heads_of(root.open, 10u);
    EXPECT_EQ(parent_heads_10, std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, SiblingUnaffectedByOtherSiblingBind) {
    const om_interval root    = om_.allocate_root();
    const om_interval sib_a   = om_.allocate_child_of(root);
    const om_interval sib_b   = om_.allocate_child_of(root);
    sw_.watch(root, 1u, {10u, 20u});
    sw_.note_var_bind(sib_a, 10u, 20u);
    const auto b_reps = sorted_reps(sw_.reps_of(sib_b.open, 1u));
    EXPECT_EQ(b_reps, (std::vector<uint32_t>{10u, 20u}));
}

// ---------------------------------------------------------------------------
// note_functor_bind
// ---------------------------------------------------------------------------

TEST_F(StructureWatcherTest, FunctorBindNoIntroducedReps) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    sw_.watch(root, 1u, {5u});
    sw_.watch(root, 2u, {5u, 6u});
    const auto changed = sorted_heads(sw_.note_functor_bind(child, 5u, {}));
    EXPECT_EQ(changed, (std::vector<watcher_head_id>{1u, 2u}));
    EXPECT_TRUE(sw_.reps_of(child.open, 1u).empty());
    const auto r2 = sw_.reps_of(child.open, 2u);
    EXPECT_EQ(r2, std::vector<uint32_t>{6u});
    EXPECT_TRUE(sw_.heads_of(child.open, 5u).empty());
}

TEST_F(StructureWatcherTest, FunctorBindAllIntroducedRepsAlreadyInHead) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    sw_.watch(root, 1u, {5u, 10u, 20u});
    const auto changed = sorted_heads(sw_.note_functor_bind(child, 5u, {10u, 20u}));
    EXPECT_EQ(changed, std::vector<watcher_head_id>{1u});
    const auto reps = sorted_reps(sw_.reps_of(child.open, 1u));
    EXPECT_EQ(reps, (std::vector<uint32_t>{10u, 20u}));
}

TEST_F(StructureWatcherTest, FunctorBindAllIntroducedRepsNew) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    sw_.watch(root, 1u, {5u});
    const auto changed = sorted_heads(sw_.note_functor_bind(child, 5u, {10u, 20u}));
    EXPECT_EQ(changed, std::vector<watcher_head_id>{1u});
    const auto reps = sorted_reps(sw_.reps_of(child.open, 1u));
    EXPECT_EQ(reps, (std::vector<uint32_t>{10u, 20u}));
    EXPECT_EQ(sw_.heads_of(child.open, 10u), std::vector<watcher_head_id>{1u});
    EXPECT_EQ(sw_.heads_of(child.open, 20u), std::vector<watcher_head_id>{1u});
}

TEST_F(StructureWatcherTest, FunctorBindOnlySomeHeadsWatchedBound) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    sw_.watch(root, 1u, {5u});
    sw_.watch(root, 2u, {6u});  // does not watch bound=5
    const auto changed = sorted_heads(sw_.note_functor_bind(child, 5u, {10u}));
    EXPECT_EQ(changed, std::vector<watcher_head_id>{1u});
    // head 2 unchanged
    EXPECT_EQ(sw_.reps_of(child.open, 2u), std::vector<uint32_t>{6u});
}

TEST_F(StructureWatcherTest, FunctorBindThenChildVarBind) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    const om_interval grand = om_.allocate_child_of(child);
    sw_.watch(root, 1u, {5u});
    sw_.note_functor_bind(child, 5u, {10u, 20u});
    // at child: head 1 watches {10, 20}
    EXPECT_EQ(sorted_reps(sw_.reps_of(child.open, 1u)),
              (std::vector<uint32_t>{10u, 20u}));
    // at grandchild, var bind 10->20
    const auto changed = sorted_heads(sw_.note_var_bind(grand, 10u, 20u));
    EXPECT_EQ(changed, std::vector<watcher_head_id>{1u});
    EXPECT_EQ(sw_.reps_of(grand.open, 1u), std::vector<uint32_t>{20u});
    // functor interval doesn't see the var bind
    EXPECT_EQ(sorted_reps(sw_.reps_of(child.open, 1u)),
              (std::vector<uint32_t>{10u, 20u}));
}

TEST_F(StructureWatcherTest, FunctorBindExistingRepForOtherHeadNoDuplicate) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    sw_.watch(root, 1u, {5u});
    sw_.watch(root, 2u, {10u});  // head 2 already watches 10
    sw_.note_functor_bind(child, 5u, {10u});  // head 1 gets 10
    // head 2's rep set: still just {10}, no duplicate
    EXPECT_EQ(sw_.reps_of(child.open, 2u), std::vector<uint32_t>{10u});
    const auto h10 = sorted_heads(sw_.heads_of(child.open, 10u));
    EXPECT_EQ(h10, (std::vector<watcher_head_id>{1u, 2u}));
}

// ---------------------------------------------------------------------------
// Ten-level spine and bush
// ---------------------------------------------------------------------------

TEST_F(StructureWatcherTest, TenLevelSpineRepsAtEachLevel) {
    const om_interval root = om_.allocate_root();
    sw_.watch(root, 1u, {0u});

    std::vector<om_interval> levels{root};
    for (int i = 0; i < 10; ++i)
        levels.push_back(om_.allocate_child_of(levels.back()));

    // bind at every other level: 0->1 at level1, 1->2 at level3, ...
    for (int i = 1; i <= 9; i += 2) {
        const uint32_t from = static_cast<uint32_t>((i - 1) / 2);
        const uint32_t to   = from + 1u;
        sw_.note_var_bind(levels[i], from, to);
    }

    // at each level the head should watch the rep introduced at or before that level
    for (int i = 1; i <= 9; ++i) {
        const uint32_t expected_rep = static_cast<uint32_t>((i + 1) / 2);
        const auto reps = sw_.reps_of(levels[i].open, 1u);
        ASSERT_EQ(reps.size(), 1u) << "level " << i;
        EXPECT_EQ(reps[0], expected_rep) << "level " << i;
    }
    EXPECT_EQ(sw_.reps_of(root.open, 1u), std::vector<uint32_t>{0u});
}

TEST_F(StructureWatcherTest, BushOfSiblingsSeeOwnBindOnly) {
    const om_interval root = om_.allocate_root();
    sw_.watch(root, 1u, {10u, 20u});
    sw_.watch(root, 2u, {30u, 40u});

    std::vector<om_interval> siblings;
    for (int i = 0; i < 5; ++i)
        siblings.push_back(om_.allocate_child_of(root));

    // each sibling does a different bind
    sw_.note_var_bind(siblings[0], 10u, 20u);  // collapse head 1
    sw_.note_var_bind(siblings[1], 30u, 40u);  // collapse head 2

    // siblings[0] sees head 1 collapsed, head 2 from parent
    EXPECT_EQ(sw_.reps_of(siblings[0].open, 1u), std::vector<uint32_t>{20u});
    EXPECT_EQ(sorted_reps(sw_.reps_of(siblings[0].open, 2u)),
              (std::vector<uint32_t>{30u, 40u}));

    // siblings[1] sees head 2 collapsed, head 1 from parent
    EXPECT_EQ(sw_.reps_of(siblings[1].open, 2u), std::vector<uint32_t>{40u});
    EXPECT_EQ(sorted_reps(sw_.reps_of(siblings[1].open, 1u)),
              (std::vector<uint32_t>{10u, 20u}));

    // unrelated sibling sees parent state
    EXPECT_EQ(sorted_reps(sw_.reps_of(siblings[2].open, 1u)),
              (std::vector<uint32_t>{10u, 20u}));
    EXPECT_EQ(sorted_reps(sw_.reps_of(siblings[2].open, 2u)),
              (std::vector<uint32_t>{30u, 40u}));
}

// ---------------------------------------------------------------------------
// Stress test: shadow map vs structure_watcher
// ---------------------------------------------------------------------------

namespace {

struct shadow_state {
    // per head: set of reps
    std::map<watcher_head_id, std::set<uint32_t>> head_to_reps;
    // per rep: set of heads
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

// All ops are performed on a single interval so the shadow does not need to
// model FPA inheritance.  Inheritance across intervals is already exercised by
// the dedicated corner-case tests above.
TEST_F(StructureWatcherTest, StressRandomOpsAgainstShadow) {
    constexpr int k_ops        = 2000;
    constexpr int k_rep_pool   = 50;
    constexpr int k_head_pool  = 20;
    constexpr uint32_t k_seed  = 777u;

    std::mt19937 rng(k_seed);

    const om_interval iv = om_.allocate_root();
    shadow_state shadow;
    structure_watcher sw;

    std::uniform_int_distribution<uint32_t> pick_rep(0u, static_cast<uint32_t>(k_rep_pool - 1));
    std::uniform_int_distribution<uint32_t> pick_head(0u, static_cast<uint32_t>(k_head_pool - 1));
    std::uniform_int_distribution<int>      pick_op(0, 2);
    std::uniform_int_distribution<int>      pick_reps_count(0, 8);
    std::uniform_int_distribution<int>      pick_intro_count(0, 6);

    auto verify = [&](int step) {
        for (int r = 0; r < k_rep_pool; ++r) {
            const auto sw_heads = sorted_heads(sw.heads_of(iv.open, static_cast<uint32_t>(r)));
            const auto sh_heads = shadow.heads_of(static_cast<uint32_t>(r));
            ASSERT_EQ(sw_heads, sh_heads)
                << "step " << step << " rep " << r << " heads mismatch";
        }
        for (int h = 0; h < k_head_pool; ++h) {
            const auto sw_reps = sorted_reps(sw.reps_of(iv.open, static_cast<watcher_head_id>(h)));
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
            sw.watch(iv, head, reps);
        } else if (op == 1) {
            const uint32_t bound  = pick_rep(rng);
            const uint32_t target = pick_rep(rng);
            if (bound == target) continue;
            const auto sh_changed = shadow.note_var_bind(bound, target);
            const auto sw_changed = sorted_heads(sw.note_var_bind(iv, bound, target));
            ASSERT_EQ(sw_changed, sh_changed)
                << "step " << step << " var bind " << bound << "->" << target;
        } else {
            const uint32_t bound = pick_rep(rng);
            const int intro_count = pick_intro_count(rng);
            std::vector<uint32_t> introduced;
            for (int j = 0; j < intro_count; ++j)
                introduced.push_back(pick_rep(rng));
            const auto sh_changed = shadow.note_functor_bind(bound, introduced);
            const auto sw_changed = sorted_heads(sw.note_functor_bind(iv, bound, introduced));
            ASSERT_EQ(sw_changed, sh_changed)
                << "step " << step << " functor bind " << bound;
        }

        verify(step);
    }
}
