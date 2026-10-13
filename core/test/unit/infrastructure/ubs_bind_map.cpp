#include <algorithm>
#include <cmath>
#include <deque>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include "ubs_bind_map_harness.hpp"

namespace {

struct immer_parent_map {
    immer::map<uint32_t, uint32_t>::transient_type& map;

    const uint32_t* get_parent(uint32_t var) const {
        return map.find(var);
    }

    void set_parent(uint32_t child, uint32_t parent) {
        map.set(child, parent);
    }
};

struct immer_size_map {
    immer::map<uint32_t, uint32_t>::transient_type& map;

    const uint32_t* get_size(uint32_t root) const {
        return map.find(root);
    }

    void set_size(uint32_t root, uint32_t class_size) {
        map.set(root, class_size);
    }
};

struct immer_value_map {
    immer::map<uint32_t, framed_expr>::transient_type& map;

    const framed_expr* get_value(uint32_t root) const {
        return map.find(root);
    }

    void set_value(uint32_t root, framed_expr value) {
        map.set(root, value);
    }
};

void build_class_of_three(ubs_bind_map_t& store) {
    store.equate(10u, 11u);
    store.equate(10u, 12u);
}

} // namespace

struct UbsBindMapTest : public ::testing::Test {
    globalizer globalize;
    ubs_bind_map_backing backing;
    ubs_bind_map_t store{backing.make_map()};

    expr func9{expr::functor{9u, {}}};
    expr func1{expr::functor{1u, {}}};
};

// --- resolve ---

TEST_F(UbsBindMapTest, ResolveIsolatedVarIsSelfAtOffsetZero) {
    const framed_expr resolved = store.resolve(7u);
    EXPECT_EQ(resolved.frame_offset, 0u);
    EXPECT_EQ(ubs_rep_index(resolved), 7u);
}

TEST_F(UbsBindMapTest, ResolveNonRootMemberReturnsClassRepresentativeNotSelf) {
    store.equate(3u, 7u);
    const framed_expr from_member = store.resolve(7u);
    const framed_expr from_root = store.resolve(3u);
    EXPECT_EQ(ubs_rep_index(from_member), 3u);
    EXPECT_EQ(ubs_rep_index(from_root), 3u);
    EXPECT_NE(ubs_rep_index(from_member), 7u);
}

TEST_F(UbsBindMapTest, ResolveDeepChainMatchesRootRepresentative) {
    store.equate(1u, 2u);
    store.equate(2u, 3u);
    store.equate(3u, 4u);
    const framed_expr shallow = store.resolve(1u);
    const framed_expr deep = store.resolve(4u);
    EXPECT_EQ(ubs_rep_index(shallow), 1u);
    EXPECT_EQ(ubs_rep_index(deep), 1u);
}

TEST_F(UbsBindMapTest, ResolveBoundClassPreservesFunctorFrameOffset) {
    expr bound{expr::functor{42u, {}}};
    store.equate(0u, 1u);
    store.instantiate(0u, {&bound, 3u});
    const framed_expr from_zero = store.resolve(0u);
    const framed_expr from_one = store.resolve(1u);
    EXPECT_EQ(from_zero.frame_offset, 3u);
    EXPECT_EQ(from_one.frame_offset, 3u);
    EXPECT_EQ(std::get<expr::functor>(from_zero.skeleton->content).id, 42u);
}

TEST_F(UbsBindMapTest, ResolveDoesNotMutateMaps) {
    const ubs_map_snapshot before = snapshot_ubs_maps(backing);
    store.resolve(4u);
    store.resolve(4u);
    store.resolve(99u);
    EXPECT_TRUE(ubs_snapshots_equal(before, snapshot_ubs_maps(backing)));
}

// --- equate ---

TEST_F(UbsBindMapTest, EquateFreshVarsResolvesToSmallerIndex) {
    store.equate(5u, 2u);
    EXPECT_EQ(ubs_rep_index(store.resolve(5u)), 2u);
    EXPECT_EQ(ubs_rep_index(store.resolve(2u)), 2u);
}

TEST_F(UbsBindMapTest, EquateSameGlobalVarIsNoOp) {
    const ubs_map_snapshot before = snapshot_ubs_maps(backing);
    store.equate(8u, 8u);
    EXPECT_TRUE(ubs_snapshots_equal(before, snapshot_ubs_maps(backing)));
}

TEST_F(UbsBindMapTest, EquateSameClassMembersIsNoOp) {
    store.equate(0u, 1u);
    const ubs_map_snapshot after_first = snapshot_ubs_maps(backing);
    store.equate(1u, 0u);
    EXPECT_TRUE(ubs_snapshots_equal(after_first, snapshot_ubs_maps(backing)));
}

TEST_F(UbsBindMapTest, EquateArgumentOrderIsIrrelevantForResolve) {
    ubs_bind_map_backing backing_ab;
    ubs_bind_map_t store_ab{backing_ab.make_map()};
    store_ab.equate(4u, 9u);

    ubs_bind_map_backing backing_ba;
    ubs_bind_map_t store_ba{backing_ba.make_map()};
    store_ba.equate(9u, 4u);

    EXPECT_EQ(ubs_rep_index(store_ab.resolve(4u)), ubs_rep_index(store_ba.resolve(4u)));
    EXPECT_EQ(ubs_rep_index(store_ab.resolve(9u)), ubs_rep_index(store_ba.resolve(9u)));
}

TEST_F(UbsBindMapTest, EquateMergesSingletonIntoLargerClassByParentLink) {
    build_class_of_three(store);
    const uint32_t singleton = 99u;
    store.equate(10u, singleton);
    EXPECT_EQ(backing.parents.map.at(singleton), 10u);
    EXPECT_EQ(ubs_rep_index(store.resolve(singleton)), 10u);
}

TEST_F(UbsBindMapTest, EquateEqualSizeClassesStillMerge) {
    store.equate(0u, 1u);
    store.equate(2u, 3u);
    store.equate(0u, 2u);
    EXPECT_EQ(ubs_rep_index(store.resolve(3u)), 0u);
    EXPECT_EQ(ubs_rep_index(store.resolve(1u)), 0u);
}

TEST_F(UbsBindMapTest, EquateSurvivorSizeIsSumIncludingImplicitSizeOne) {
    store.equate(0u, 1u);
    const uint32_t root = ubs_find_root(backing.parents, 0u);
    EXPECT_EQ(backing.sizes.map.at(root), 2u);

    store.equate(0u, 2u);
    EXPECT_EQ(backing.sizes.map.at(root), 3u);
}

TEST_F(UbsBindMapTest, EquateRepresentativeMinWhenCalleeRootSurvivesBySize) {
    store.equate(5u, 3u);
    EXPECT_EQ(ubs_find_root(backing.parents, 3u), 5u);
    EXPECT_EQ(ubs_rep_index(store.resolve(5u)), 3u);
    EXPECT_EQ(ubs_rep_index(store.resolve(3u)), 3u);
}

TEST_F(UbsBindMapTest, EquateRandomMergeOrderTenVarsShareMinimum) {
    std::vector<uint32_t> order{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::mt19937 rng(12345u);
    std::shuffle(order.begin(), order.end(), rng);
    for (size_t i = 1; i < order.size(); ++i)
        store.equate(order[0], order[i]);
    for (uint32_t var = 0; var < 10u; ++var)
        EXPECT_EQ(ubs_rep_index(store.resolve(var)), 0u);
}

TEST_F(UbsBindMapTest, EquateTransitivity) {
    store.equate(6u, 7u);
    store.equate(7u, 8u);
    EXPECT_EQ(ubs_rep_index(store.resolve(6u)), ubs_rep_index(store.resolve(8u)));
}

TEST_F(UbsBindMapTest, EquateIntoBoundClassThrowsInDebug) {
    expr bound{expr::functor{1u, {}}};
    store.instantiate(0u, {&bound, 0});
    EXPECT_THROW(store.equate(0u, 1u), std::logic_error);
}

// --- instantiate ---

TEST_F(UbsBindMapTest, InstantiateFreshVarResolvesToFunctor) {
    store.instantiate(4u, {&func9, 0});
    EXPECT_EQ(std::get<expr::functor>(store.resolve(4u).skeleton->content).id, 9u);
}

TEST_F(UbsBindMapTest, InstantiateClassMemberBindsWholeClass) {
    store.equate(0u, 1u);
    store.equate(1u, 2u);
    store.instantiate(2u, {&func1, 2u});
    EXPECT_EQ(store.resolve(0u).frame_offset, 2u);
    EXPECT_EQ(store.resolve(1u).frame_offset, 2u);
    EXPECT_EQ(std::get<expr::functor>(store.resolve(2u).skeleton->content).id, 1u);
}

TEST_F(UbsBindMapTest, InstantiateThroughNonRootWritesValueAtRoot) {
    store.equate(5u, 3u);
    store.instantiate(3u, {&func9, 0});
    const uint32_t root = ubs_find_root(backing.parents, 3u);
    EXPECT_TRUE(backing.values.map.count(root) != 0);
    EXPECT_EQ(backing.values.map.count(3u), 0u);
}

TEST_F(UbsBindMapTest, InstantiateAlreadyBoundClassThrowsInDebug) {
    store.instantiate(1u, {&func9, 0});
    EXPECT_THROW(store.instantiate(1u, {&func1, 0}), std::logic_error);
}

TEST_F(UbsBindMapTest, InstantiateWithVariableSkeletonThrowsInDebug) {
    expr not_functor{expr::var{0}};
    EXPECT_THROW(store.instantiate(2u, {&not_functor, 0}), std::logic_error);
}

TEST_F(UbsBindMapTest, InstantiateDoesNotAddParentLinks) {
    const size_t parents_before = backing.parents.map.size();
    store.instantiate(8u, {&func9, 0});
    EXPECT_EQ(backing.parents.map.size(), parents_before);
}

// --- union by size ---

TEST_F(UbsBindMapTest, UnionBySizeChainHeightBoundedByLogN) {
    std::vector<uint32_t> frontier;
    for (uint32_t i = 0; i < 16u; ++i)
        frontier.push_back(i);
    while (frontier.size() > 1u) {
        std::vector<uint32_t> next;
        for (size_t i = 0; i + 1 < frontier.size(); i += 2u) {
            store.equate(frontier[i], frontier[i + 1]);
            next.push_back(frontier[i]);
        }
        if (frontier.size() % 2u == 1u)
            next.push_back(frontier.back());
        frontier = std::move(next);
    }
    uint32_t max_depth = 0;
    for (uint32_t var = 0; var < 16u; ++var) {
        const uint32_t depth = ubs_parent_chain_depth(backing.parents, var);
        max_depth = std::max(max_depth, depth);
    }
    const uint32_t bound = static_cast<uint32_t>(std::ceil(std::log2(16.0)));
    EXPECT_LE(max_depth, bound);
}

TEST_F(UbsBindMapTest, UnionBySizeAdversarialSingletonIntoBigStaysShallow) {
    build_class_of_three(store);
    for (uint32_t singleton = 20u; singleton < 30u; ++singleton) {
        store.equate(10u, singleton);
        EXPECT_LE(ubs_parent_chain_depth(backing.parents, singleton), 1u);
    }
}

TEST_F(UbsBindMapTest, UnionBySizeRandomMergesTerminateWithoutCycle) {
    std::mt19937 rng(42);
    for (int round = 0; round < 50; ++round) {
        const uint32_t a = static_cast<uint32_t>(rng() % 20u);
        const uint32_t b = static_cast<uint32_t>(rng() % 20u);
        store.equate(a, b);
    }
    for (uint32_t var = 0; var < 20u; ++var) {
        uint32_t walk = var;
        std::unordered_map<uint32_t, bool> seen;
        while (const uint32_t* parent = backing.parents.get_parent(walk)) {
            EXPECT_FALSE(seen[walk]);
            seen[walk] = true;
            walk = *parent;
        }
    }
}

TEST_F(UbsBindMapTest, UnionBySizeParentChainsEndAtClassRoot) {
    store.equate(1u, 2u);
    store.equate(3u, 4u);
    store.equate(1u, 3u);
    for (uint32_t var = 1; var <= 4u; ++var) {
        const uint32_t root = ubs_find_root(backing.parents, var);
        EXPECT_FALSE(backing.parents.get_parent(root));
        EXPECT_EQ(ubs_rep_index(store.resolve(var)), ubs_rep_index(store.resolve(root)));
    }
}

TEST_F(UbsBindMapTest, StaleLoserMetadataIgnoredAfterMerge) {
    store.equate(5u, 3u);
    backing.sizes.map[3u] = 999u;
    backing.values.map[3u] = framed_expr{backing.vars.make(888u), 0u};
    EXPECT_EQ(ubs_rep_index(store.resolve(3u)), 3u);
    EXPECT_EQ(ubs_rep_index(store.resolve(5u)), 3u);
}

TEST(UbsBindMapImmerTransientTest, EquateAndInstantiateOnTransientMaps) {
    ubs_var_factory vars;
    auto parents = immer::map<uint32_t, uint32_t>{}.transient();
    auto sizes = immer::map<uint32_t, uint32_t>{}.transient();
    auto values = immer::map<uint32_t, framed_expr>{}.transient();

    immer_parent_map parent_access{parents};
    immer_size_map size_access{sizes};
    immer_value_map value_access{values};

    ubs_bind_map<
        ubs_var_factory,
        immer_parent_map,
        immer_parent_map,
        immer_size_map,
        immer_size_map,
        immer_value_map,
        immer_value_map> store{vars, parent_access, parent_access, size_access, size_access, value_access, value_access};

    expr bound{expr::functor{11u, {}}};
    store.equate(0u, 1u);
    store.instantiate(0u, {&bound, 0});
    EXPECT_EQ(std::get<expr::functor>(store.resolve(1u).skeleton->content).id, 11u);
}
