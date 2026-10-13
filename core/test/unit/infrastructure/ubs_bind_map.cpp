#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/ubs_bind_map.hpp"
#include "ubs_bind_map_test_maps.hpp"

namespace {

struct MockMakeVar {
    MOCK_METHOD(const expr*, make, (uint32_t));
};

using test_ubs_bind_map_t = ubs_bind_map<
    MockMakeVar,
    ubs_std_parent_map,
    ubs_std_parent_map,
    ubs_std_size_map,
    ubs_std_size_map,
    ubs_std_value_map,
    ubs_std_value_map>;

struct UbsBindMapTest : public ::testing::Test {
    ::testing::NiceMock<MockMakeVar> make_var;
    ubs_std_parent_map parents;
    ubs_std_size_map sizes;
    ubs_std_value_map values;
    test_ubs_bind_map_t store{make_var, parents, parents, sizes, sizes, values, values};

    std::deque<expr> owned_exprs;
    std::unordered_map<uint32_t, const expr*> vars_by_index;

    const expr* var_node(uint32_t index) {
        const auto found = vars_by_index.find(index);
        if (found != vars_by_index.end())
            return found->second;
        owned_exprs.push_back(expr{expr::var{index}});
        const expr* node = &owned_exprs.back();
        vars_by_index.emplace(index, node);
        return node;
    }

    const expr* functor_node(uint32_t id) {
        owned_exprs.push_back(expr{expr::functor{id, {}}});
        return &owned_exprs.back();
    }

    void SetUp() override {
        ON_CALL(make_var, make(testing::_)).WillByDefault(testing::Invoke(this, &UbsBindMapTest::var_node));
    }

    ubs_map_snapshot snapshot() const {
        return snapshot_ubs_maps(parents, sizes, values);
    }
};

void build_class_of_three(test_ubs_bind_map_t& store) {
    store.equate(10u, 11u);
    store.equate(10u, 12u);
}

} // namespace

TEST_F(UbsBindMapTest, ResolveIsolatedVarDelegatesToMakeVar) {
    const expr* expected = var_node(7u);
    EXPECT_CALL(make_var, make(7u)).WillOnce(testing::Return(expected));
    EXPECT_EQ(store.resolve(7u), expected);
}

TEST_F(UbsBindMapTest, ResolveNonRootMemberReturnsRepresentativeNode) {
    const expr* rep = var_node(3u);
    store.equate(3u, 7u);
    EXPECT_EQ(store.resolve(7u), rep);
    EXPECT_EQ(store.resolve(3u), rep);
}

TEST_F(UbsBindMapTest, ResolveDeepChainMatchesRoot) {
    store.equate(1u, 2u);
    store.equate(2u, 3u);
    store.equate(3u, 4u);
    const expr* rep = var_node(1u);
    EXPECT_EQ(store.resolve(4u), rep);
}

TEST_F(UbsBindMapTest, ResolveBoundClassSharesStoredPointer) {
    const expr* bound = functor_node(42u);
    store.equate(0u, 1u);
    store.instantiate(0u, bound);
    EXPECT_EQ(store.resolve(0u), bound);
    EXPECT_EQ(store.resolve(1u), bound);
}

TEST_F(UbsBindMapTest, ResolveDoesNotMutateMaps) {
    const ubs_map_snapshot before = snapshot();
    EXPECT_CALL(make_var, make(4u)).WillOnce(testing::Return(var_node(4u)));
    store.resolve(4u);
    EXPECT_TRUE(ubs_snapshots_equal(before, snapshot()));
}

TEST_F(UbsBindMapTest, EquateFreshVarsResolveToSmallerIndex) {
    store.equate(5u, 2u);
    EXPECT_EQ(ubs_rep_index(store.resolve(5u)), 2u);
    EXPECT_EQ(ubs_rep_index(store.resolve(2u)), 2u);
}

TEST_F(UbsBindMapTest, EquateSameVarOrSameClassMembersIsNoOp) {
    const ubs_map_snapshot before = snapshot();
    store.equate(8u, 8u);
    EXPECT_TRUE(ubs_snapshots_equal(before, snapshot()));
    store.equate(0u, 1u);
    const ubs_map_snapshot after_merge = snapshot();
    store.equate(1u, 0u);
    EXPECT_TRUE(ubs_snapshots_equal(after_merge, snapshot()));
}

TEST_F(UbsBindMapTest, EquateArgumentOrderIsIrrelevant) {
    ::testing::NiceMock<MockMakeVar> make_var_ab;
    ::testing::NiceMock<MockMakeVar> make_var_ba;
    UbsBindMapTest* self_ab = this;
    UbsBindMapTest* self_ba = this;
    ON_CALL(make_var_ab, make(testing::_)).WillByDefault(testing::Invoke(self_ab, &UbsBindMapTest::var_node));
    ON_CALL(make_var_ba, make(testing::_)).WillByDefault(testing::Invoke(self_ba, &UbsBindMapTest::var_node));
    ubs_std_parent_map parents_ab;
    ubs_std_parent_map parents_ba;
    ubs_std_size_map sizes_ab;
    ubs_std_size_map sizes_ba;
    ubs_std_value_map values_ab;
    ubs_std_value_map values_ba;
    test_ubs_bind_map_t store_ab{make_var_ab, parents_ab, parents_ab, sizes_ab, sizes_ab, values_ab, values_ab};
    test_ubs_bind_map_t store_ba{make_var_ba, parents_ba, parents_ba, sizes_ba, sizes_ba, values_ba, values_ba};
    store_ab.equate(4u, 9u);
    store_ba.equate(9u, 4u);
    EXPECT_EQ(ubs_rep_index(store_ab.resolve(4u)), ubs_rep_index(store_ba.resolve(4u)));
}

TEST_F(UbsBindMapTest, EquateLargerClassRootSurvives) {
    build_class_of_three(store);
    store.equate(10u, 99u);
    EXPECT_EQ(parents.map.at(99u), 10u);
}

TEST_F(UbsBindMapTest, EquateEqualSizeClassesMergeCorrectly) {
    store.equate(0u, 1u);
    store.equate(2u, 3u);
    store.equate(0u, 2u);
    EXPECT_EQ(ubs_rep_index(store.resolve(3u)), 0u);
}

TEST_F(UbsBindMapTest, EquateSurvivorSizeIsSumWithImplicitOnes) {
    store.equate(0u, 1u);
    const uint32_t root = ubs_find_root(parents, 0u);
    EXPECT_EQ(sizes.map.at(root), 2u);
    store.equate(0u, 2u);
    EXPECT_EQ(sizes.map.at(root), 3u);
}

TEST_F(UbsBindMapTest, EquateRepresentativeMinWhenLargerRootSurvives) {
    store.equate(5u, 3u);
    EXPECT_EQ(ubs_rep_index(store.resolve(5u)), 3u);
}

TEST_F(UbsBindMapTest, EquateRandomMergesShareOverallMinimum) {
    std::vector<uint32_t> order{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::mt19937 rng(12345u);
    std::shuffle(order.begin(), order.end(), rng);
    for (size_t i = 1; i < order.size(); ++i)
        store.equate(order[0], order[i]);
    for (uint32_t var = 0; var < 10u; ++var)
        EXPECT_EQ(ubs_rep_index(store.resolve(var)), 0u);
}

#ifdef DEBUG
TEST_F(UbsBindMapTest, EquateIntoBoundClassThrows) {
    store.instantiate(0u, functor_node(1u));
    EXPECT_THROW(store.equate(0u, 1u), std::logic_error);
}

TEST_F(UbsBindMapTest, EquateWithBoundSideThrows) {
    store.instantiate(2u, functor_node(1u));
    EXPECT_THROW(store.equate(0u, 2u), std::logic_error);
}

TEST_F(UbsBindMapTest, EquateMemberOfBoundClassThrows) {
    store.equate(0u, 1u);
    store.instantiate(0u, functor_node(1u));
    EXPECT_THROW(store.equate(1u, 2u), std::logic_error);
}
#endif

TEST_F(UbsBindMapTest, InstantiateAnyMemberBindsWholeClass) {
    store.equate(0u, 1u);
    store.equate(1u, 2u);
    const expr* bound = functor_node(1u);
    store.instantiate(2u, bound);
    EXPECT_EQ(store.resolve(0u), bound);
}

TEST_F(UbsBindMapTest, InstantiateThroughNonRootWritesAtRootOnly) {
    store.equate(5u, 3u);
    const expr* bound = functor_node(9u);
    store.instantiate(3u, bound);
    const uint32_t root = ubs_find_root(parents, 3u);
    EXPECT_EQ(values.map.at(root), bound);
    EXPECT_EQ(values.map.count(3u), 0u);
}

#ifdef DEBUG
TEST_F(UbsBindMapTest, InstantiateBoundClassThrows) {
    store.instantiate(1u, functor_node(9u));
    EXPECT_THROW(store.instantiate(1u, functor_node(1u)), std::logic_error);
}

TEST_F(UbsBindMapTest, InstantiateMergedBoundClassThrows) {
    store.equate(0u, 1u);
    store.instantiate(0u, functor_node(9u));
    EXPECT_THROW(store.instantiate(1u, functor_node(1u)), std::logic_error);
}

TEST_F(UbsBindMapTest, InstantiateVariableNodeThrows) {
    EXPECT_THROW(store.instantiate(2u, var_node(0u)), std::logic_error);
}
#endif

TEST_F(UbsBindMapTest, UnionBySizeHeightBoundedByLogN) {
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
    for (uint32_t var = 0; var < 16u; ++var)
        max_depth = std::max(max_depth, ubs_parent_chain_depth(parents, var));
    EXPECT_LE(max_depth, static_cast<uint32_t>(std::ceil(std::log2(16.0))));
}

TEST_F(UbsBindMapTest, UnionBySizeAdversarialOrdersStayShallow) {
    build_class_of_three(store);
    for (uint32_t singleton = 20u; singleton < 30u; ++singleton)
        EXPECT_LE(ubs_parent_chain_depth(parents, singleton), 1u);
}

TEST_F(UbsBindMapTest, UnionBySizeNoCyclesAndChainsStayInClass) {
    std::mt19937 rng(42);
    for (int round = 0; round < 50; ++round)
        store.equate(static_cast<uint32_t>(rng() % 20u), static_cast<uint32_t>(rng() % 20u));
    for (uint32_t var = 0; var < 20u; ++var) {
        const expr* rep = store.resolve(var);
        uint32_t walk = var;
        while (const uint32_t* parent = parents.get_parent(walk)) {
            walk = *parent;
            EXPECT_EQ(store.resolve(walk), rep);
        }
    }
}

TEST_F(UbsBindMapTest, StaleNonRootMetadataIgnored) {
    store.equate(5u, 3u);
    sizes.map[3u] = 999u;
    values.map[3u] = var_node(888u);
    EXPECT_EQ(ubs_rep_index(store.resolve(3u)), 3u);
}

TEST_F(UbsBindMapTest, ResolveIndexZeroUsesMakeVarNotOffByOne) {
    const expr* zero = var_node(0u);
    EXPECT_CALL(make_var, make(0u)).WillOnce(testing::Return(zero));
    EXPECT_EQ(store.resolve(0u), zero);
    EXPECT_EQ(parents.map.count(0u), 0u);
}

TEST_F(UbsBindMapTest, ResolveHugeFreshIndexDelegatesToMakeVar) {
    const uint32_t huge = UINT32_MAX;
    const expr* node = var_node(huge);
    EXPECT_CALL(make_var, make(huge)).WillOnce(testing::Return(node));
    EXPECT_EQ(store.resolve(huge), node);
}

TEST_F(UbsBindMapTest, MergedNeverBoundStillResolvesToVarNotFunctor) {
    store.equate(2u, 5u);
    const expr* rep = store.resolve(2u);
    EXPECT_TRUE(std::holds_alternative<expr::var>(rep->content));
    EXPECT_EQ(ubs_rep_index(rep), 2u);
}

TEST_F(UbsBindMapTest, EquateSingletonIntoLargeClassGrowsSizeByOne) {
    build_class_of_three(store);
    const uint32_t large_root = ubs_find_root(parents, 10u);
    const uint32_t size_before = sizes.map.at(large_root);
    store.equate(10u, 50u);
    EXPECT_EQ(ubs_find_root(parents, 50u), large_root);
    EXPECT_EQ(sizes.map.at(large_root), size_before + 1u);
}

TEST_F(UbsBindMapTest, EquateTwoSizeTwoClassesYieldsSizeFour) {
    store.equate(0u, 1u);
    store.equate(2u, 3u);
    store.equate(0u, 2u);
    const uint32_t root = ubs_find_root(parents, 0u);
    EXPECT_EQ(sizes.map.at(root), 4u);
    EXPECT_EQ(parents.map.count(root), 0u);
}

TEST_F(UbsBindMapTest, EquateWhenLosingRootIsSmallerIndexRepresentative) {
    store.equate(1u, 0u);
    store.equate(2u, 3u);
    store.equate(3u, 1u);
    EXPECT_EQ(ubs_rep_index(store.resolve(2u)), 0u);
}

TEST_F(UbsBindMapTest, EquateWhenBothRepsEqualTheirRoots) {
    store.equate(4u, 9u);
    const uint32_t root = ubs_find_root(parents, 4u);
    EXPECT_EQ(ubs_rep_index(store.resolve(9u)), 4u);
    const expr* stored = values.map.at(root);
    EXPECT_EQ(ubs_rep_index(stored), 4u);
}

TEST_F(UbsBindMapTest, EquateWhenMinIndexDeepInChain) {
    store.equate(8u, 7u);
    store.equate(7u, 6u);
    store.equate(6u, 5u);
    EXPECT_EQ(ubs_rep_index(store.resolve(8u)), 5u);
}

TEST_F(UbsBindMapTest, EquateChainIncreasingIndexKeepsFirstRepresentative) {
    for (uint32_t next = 1u; next < 8u; ++next)
        store.equate(0u, next);
    for (uint32_t member = 0u; member < 8u; ++member)
        EXPECT_EQ(ubs_rep_index(store.resolve(member)), 0u);
}

TEST_F(UbsBindMapTest, EquateChainDecreasingIndexTracksNewestMinimum) {
    store.equate(5u, 4u);
    EXPECT_EQ(ubs_rep_index(store.resolve(5u)), 4u);
    store.equate(4u, 3u);
    EXPECT_EQ(ubs_rep_index(store.resolve(5u)), 3u);
    store.equate(3u, 2u);
    EXPECT_EQ(ubs_rep_index(store.resolve(4u)), 2u);
    store.equate(2u, 1u);
    EXPECT_EQ(ubs_rep_index(store.resolve(3u)), 1u);
    store.equate(1u, 0u);
    EXPECT_EQ(ubs_rep_index(store.resolve(2u)), 0u);
}

TEST_F(UbsBindMapTest, EquateSamePairManyTimesChangesSizeOnlyOnce) {
    const ubs_map_snapshot before = snapshot();
    store.equate(1u, 2u);
    const ubs_map_snapshot after_first = snapshot();
    EXPECT_NE(before.parents, after_first.parents);
    for (int repeat = 0; repeat < 20; ++repeat)
        store.equate(1u, 2u);
    EXPECT_TRUE(ubs_snapshots_equal(after_first, snapshot()));
}

TEST_F(UbsBindMapTest, EquateAbThenBaDoesNotDoubleClassSize) {
    store.equate(1u, 2u);
    const ubs_map_snapshot after_ab = snapshot();
    store.equate(2u, 1u);
    EXPECT_TRUE(ubs_snapshots_equal(after_ab, snapshot()));
    const uint32_t root = ubs_find_root(parents, 1u);
    EXPECT_EQ(sizes.map.at(root), 2u);
}

TEST_F(UbsBindMapTest, EquateTwoClassesWithCallerVarsKeepsSmallerCaller) {
    store.equate(4u, 7u);
    store.equate(3u, 9u);
    store.equate(4u, 3u);
    EXPECT_EQ(ubs_rep_index(store.resolve(7u)), 3u);
    EXPECT_EQ(ubs_rep_index(store.resolve(9u)), 3u);
}

TEST_F(UbsBindMapTest, EquateMergeWritesNoParentForSurvivor) {
    const size_t parents_before = parents.map.size();
    store.equate(10u, 2u);
    const uint32_t survivor = ubs_find_root(parents, 10u);
    EXPECT_EQ(parents.map.count(survivor), 0u);
    EXPECT_EQ(parents.map.size(), parents_before + 1u);
}

TEST_F(UbsBindMapTest, EquateMergeWritesExactlyOneParentEntry) {
    const ubs_map_snapshot before = snapshot();
    store.equate(6u, 11u);
    const ubs_map_snapshot after = snapshot();
    EXPECT_EQ(after.parents.size(), before.parents.size() + 1u);
}

TEST_F(UbsBindMapTest, InstantiateZeroArityFunctor) {
    const expr* constant = functor_node(0u);
    store.instantiate(3u, constant);
    EXPECT_EQ(store.resolve(3u), constant);
}

TEST_F(UbsBindMapTest, InstantiateDeepNestedStoresExactPointer) {
    const expr* inner = functor_node(1u);
    owned_exprs.push_back(expr{expr::functor{2u, {inner}}});
    const expr* outer = &owned_exprs.back();
    store.instantiate(0u, outer);
    EXPECT_EQ(store.resolve(0u), outer);
}

TEST_F(UbsBindMapTest, InstantiateSingletonWritesNoParentOrSize) {
    const ubs_map_snapshot before = snapshot();
    store.instantiate(4u, functor_node(5u));
    const ubs_map_snapshot after = snapshot();
    EXPECT_EQ(after.parents, before.parents);
    EXPECT_EQ(after.sizes, before.sizes);
    EXPECT_EQ(after.values.size(), before.values.size() + 1u);
}

TEST_F(UbsBindMapTest, InstantiateMergedClassLeavesStaleSizeUntouched) {
    store.equate(1u, 2u);
    const uint32_t root = ubs_find_root(parents, 1u);
    const uint32_t size_before = sizes.map.at(root);
    store.instantiate(2u, functor_node(8u));
    EXPECT_EQ(sizes.map.at(root), size_before);
}
