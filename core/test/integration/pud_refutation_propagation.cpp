#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_children.hpp"
#include "infrastructure/pud_leaf_nodes.hpp"
#include "infrastructure/pud_live_leaf_finder.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "infrastructure/pud_node_parents.hpp"
#include "infrastructure/pud_refutation_propagator.hpp"
#include "infrastructure/pud_refuted_node_checker.hpp"
#include "infrastructure/pud_root_nodes.hpp"
#include "infrastructure/pud_witness_search_head.hpp"
#include "infrastructure/pud_witness_search_head_factory.hpp"
#include "infrastructure/pud_witness_search_head_forker.hpp"

using ::testing::_;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::UnorderedElementsAre;

namespace {

using child_iter = std::vector<pud_node_id>::const_iterator;

struct descent_t {
    pud_node_id node;
    bool operator==(const descent_t&) const = default;
};

struct MockDescend {
    MOCK_METHOD(std::optional<descent_t>, descend, (descent_t, pud_node_id));
};

struct MockCallSite {
    MOCK_METHOD(size_t, get, (pud_node_id));
};

using checker_t = pud_refuted_node_checker<pud_node_live_child_counts>;
using head_t = pud_witness_search_head<
    descent_t, child_iter, pud_leaf_nodes, checker_t, pud_children, NiceMock<MockDescend>, NiceMock<MockCallSite>>;
using factory_t = pud_witness_search_head_factory<
    descent_t, child_iter, pud_leaf_nodes, checker_t, pud_children, NiceMock<MockDescend>, NiceMock<MockCallSite>>;
using forker_t = pud_witness_search_head_forker<
    descent_t, child_iter, pud_leaf_nodes, checker_t, pud_children, NiceMock<MockDescend>, NiceMock<MockCallSite>>;
using mhws_t = pud_mhws<descent_t, child_iter, head_t, factory_t, forker_t>;
using finder_t = pud_live_leaf_finder<checker_t, pud_leaf_nodes, pud_children>;
using propagator_t = pud_refutation_propagator<pud_node_live_child_counts, pud_root_nodes, pud_node_parents>;

}  // namespace

// root -> {subtree, other}; subtree -> {leaf_a, middle}; middle -> {leaf_b, leaf_c};
// other -> {leaf_d}.
struct PudRefutationPropagationIntegrationTest : public ::testing::Test {
    pud_leaf_nodes leaves;
    pud_children children;
    pud_node_parents parents;
    pud_root_nodes root_nodes;
    pud_node_live_child_counts live_child_counts;
    checker_t refuted_nodes{live_child_counts};
    NiceMock<MockDescend> descend;
    NiceMock<MockCallSite> call_sites;
    factory_t factory{leaves, refuted_nodes, children, descend, call_sites};
    forker_t forker;
    mhws_t searches{factory, forker};
    finder_t finder{refuted_nodes, leaves, children};
    propagator_t propagator{live_child_counts, root_nodes, parents};
    pud_node_id root = 1;
    pud_node_id subtree = 2;
    pud_node_id other = 3;
    pud_node_id leaf_a = 4;
    pud_node_id middle = 5;
    pud_node_id leaf_b = 6;
    pud_node_id leaf_c = 7;
    pud_node_id leaf_d = 8;

    void link(pud_node_id parent, std::vector<pud_node_id> node_children) {
        live_child_counts.store(parent, node_children.size());
        for (pud_node_id child : node_children)
            parents.store(child, parent);
        children.store(parent, std::move(node_children));
    }

    void make_leaf(pud_node_id leaf) {
        leaves.set_leaf(leaf);
        live_child_counts.store(leaf, 1);
    }

    void SetUp() override {
        ON_CALL(descend, descend(_, _)).WillByDefault([](descent_t, pud_node_id child) {
            return std::optional<descent_t>{descent_t{child}};
        });
        root_nodes.mark_root(root);
        link(root, {subtree, other});
        link(subtree, {leaf_a, middle});
        link(middle, {leaf_b, leaf_c});
        link(other, {leaf_d});
        make_leaf(leaf_a);
        make_leaf(leaf_b);
        make_leaf(leaf_c);
        make_leaf(leaf_d);
    }

    std::vector<pud_mhws_head_id> refute_and_invalidate(pud_node_id node) {
        const std::vector<pud_node_id> live_leaves = finder.find_live_leaves(node);
        for (pud_node_id leaf : live_leaves)
            propagator.refute(leaf);
        std::vector<pud_mhws_head_id> exhausted;
        for (pud_node_id leaf : live_leaves) {
            const std::vector<pud_mhws_head_id> leaf_exhausted = searches.invalidate_leaf(leaf);
            exhausted.insert(exhausted.end(), leaf_exhausted.begin(), leaf_exhausted.end());
        }
        return exhausted;
    }
};

// ---------- refuting ----------

TEST_F(PudRefutationPropagationIntegrationTest, NothingIsRefutedInitially) {
    for (pud_node_id node : {root, subtree, other, leaf_a, middle, leaf_b, leaf_c, leaf_d})
        EXPECT_FALSE(refuted_nodes.check_refuted(node));
}

TEST_F(PudRefutationPropagationIntegrationTest, RefutingASubtreesLeavesRefutesTheWholeSubtreeAndNothingElse) {
    refute_and_invalidate(subtree);
    for (pud_node_id node : {subtree, leaf_a, middle, leaf_b, leaf_c})
        EXPECT_TRUE(refuted_nodes.check_refuted(node));
    for (pud_node_id node : {root, other, leaf_d})
        EXPECT_FALSE(refuted_nodes.check_refuted(node));
}

TEST_F(PudRefutationPropagationIntegrationTest, FinderReturnsTheSubtreesLiveLeaves) {
    EXPECT_THAT(finder.find_live_leaves(subtree), UnorderedElementsAre(leaf_a, leaf_b, leaf_c));
}

TEST_F(PudRefutationPropagationIntegrationTest, RefutedSubtreeHasNoLiveLeavesLeft) {
    refute_and_invalidate(subtree);
    EXPECT_THAT(finder.find_live_leaves(subtree), IsEmpty());
    EXPECT_THAT(finder.find_live_leaves(root), UnorderedElementsAre(leaf_d));
}

TEST_F(PudRefutationPropagationIntegrationTest, LastLiveLeafRefutesItsAncestorsUpToALiveSibling) {
    propagator.refute(leaf_b);
    EXPECT_FALSE(refuted_nodes.check_refuted(middle));
    propagator.refute(leaf_c);
    EXPECT_TRUE(refuted_nodes.check_refuted(middle));
    EXPECT_FALSE(refuted_nodes.check_refuted(subtree));
    propagator.refute(leaf_a);
    EXPECT_TRUE(refuted_nodes.check_refuted(subtree));
    EXPECT_FALSE(refuted_nodes.check_refuted(root));
}

TEST_F(PudRefutationPropagationIntegrationTest, RefutingEverySubtreeRefutesTheRoot) {
    refute_and_invalidate(subtree);
    propagator.refute(leaf_d);
    EXPECT_TRUE(refuted_nodes.check_refuted(other));
    EXPECT_TRUE(refuted_nodes.check_refuted(root));
}

TEST_F(PudRefutationPropagationIntegrationTest, RefutingAPartlyRefutedSubtreeOnlyRefutesItsLiveLeaves) {
    propagator.refute(leaf_b);
    EXPECT_THAT(finder.find_live_leaves(subtree), UnorderedElementsAre(leaf_a, leaf_c));
    refute_and_invalidate(subtree);
    EXPECT_TRUE(refuted_nodes.check_refuted(subtree));
    EXPECT_FALSE(refuted_nodes.check_refuted(root));
}

// ---------- witness searches ----------

TEST_F(PudRefutationPropagationIntegrationTest, SearchesRootedInsideTheSubtreeRunOut) {
    const pud_mhws_head_id at_subtree = searches.try_add_head(descent_t{subtree}).value();
    const pud_mhws_head_id at_middle = searches.try_add_head(descent_t{middle}).value();
    EXPECT_THAT(refute_and_invalidate(subtree), UnorderedElementsAre(at_subtree, at_middle));
}

TEST_F(PudRefutationPropagationIntegrationTest, SearchAboveTheSubtreeMovesToALiveLeafOutsideIt) {
    const pud_mhws_head_id at_root = searches.try_add_head(descent_t{root}).value();
    EXPECT_THAT(refute_and_invalidate(subtree), IsEmpty());
    EXPECT_THAT(refute_and_invalidate(other), UnorderedElementsAre(at_root));
}

TEST_F(PudRefutationPropagationIntegrationTest, SearchOutsideTheSubtreeIsUntouched) {
    const pud_mhws_head_id at_other = searches.try_add_head(descent_t{other}).value();
    EXPECT_THAT(refute_and_invalidate(subtree), IsEmpty());
    EXPECT_THAT(refute_and_invalidate(other), UnorderedElementsAre(at_other));
}

TEST_F(PudRefutationPropagationIntegrationTest, NewSearchInsideTheRefutedSubtreeFindsNothing) {
    refute_and_invalidate(subtree);
    EXPECT_FALSE(searches.try_add_head(descent_t{subtree}).has_value());
    EXPECT_FALSE(searches.try_add_head(descent_t{middle}).has_value());
    EXPECT_FALSE(searches.try_add_head(descent_t{leaf_b}).has_value());
}

TEST_F(PudRefutationPropagationIntegrationTest, NewSearchAboveTheRefutedSubtreeFindsTheLiveLeaf) {
    refute_and_invalidate(subtree);
    const pud_mhws_head_id at_root = searches.try_add_head(descent_t{root}).value();
    EXPECT_THAT(refute_and_invalidate(other), UnorderedElementsAre(at_root));
}

TEST_F(PudRefutationPropagationIntegrationTest, ForkedSearchSkipsARefutedSubtreeBeforeItsLeavesAreInvalidated) {
    const pud_mhws_head_id at_root = searches.try_add_head(descent_t{root}).value();
    for (pud_node_id leaf : finder.find_live_leaves(subtree))
        propagator.refute(leaf);
    const pud_mhws_head_id forked = searches.try_fork_head(at_root, descent_t{root}).value();
    EXPECT_THAT(refute_and_invalidate(other), UnorderedElementsAre(forked));
}

TEST_F(PudRefutationPropagationIntegrationTest, RefutingTheRootExhaustsEverySearch) {
    const pud_mhws_head_id at_root = searches.try_add_head(descent_t{root}).value();
    const pud_mhws_head_id at_middle = searches.try_add_head(descent_t{middle}).value();
    const pud_mhws_head_id at_other = searches.try_add_head(descent_t{other}).value();
    EXPECT_THAT(refute_and_invalidate(root), UnorderedElementsAre(at_root, at_middle, at_other));
    EXPECT_TRUE(refuted_nodes.check_refuted(root));
}
