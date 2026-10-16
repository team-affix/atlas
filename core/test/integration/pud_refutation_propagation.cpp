#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_children.hpp"
#include "infrastructure/pud_leaf_nodes.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_node_parents.hpp"
#include "infrastructure/pud_refutation_propagator.hpp"
#include "infrastructure/pud_refuted_leaf_recorder.hpp"
#include "infrastructure/pud_refuted_nodes.hpp"
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

using head_t = pud_witness_search_head<
    descent_t, child_iter, pud_leaf_nodes, pud_refuted_nodes, pud_children, NiceMock<MockDescend>, NiceMock<MockCallSite>>;
using factory_t = pud_witness_search_head_factory<
    descent_t, child_iter, pud_leaf_nodes, pud_refuted_nodes, pud_children, NiceMock<MockDescend>, NiceMock<MockCallSite>>;
using forker_t = pud_witness_search_head_forker<
    descent_t, child_iter, pud_leaf_nodes, pud_refuted_nodes, pud_children, NiceMock<MockDescend>, NiceMock<MockCallSite>>;
using mhws_t = pud_mhws<descent_t, child_iter, head_t, factory_t, forker_t>;
using recorder_t = pud_refuted_leaf_recorder<pud_refuted_nodes, pud_leaf_nodes>;
using propagator_t = pud_refutation_propagator<pud_refuted_nodes, recorder_t, pud_children, pud_node_parents, pud_root_nodes>;

}  // namespace

// root -> {refuted_root, other}; refuted_root -> {leaf_a, middle}; middle -> {leaf_b, leaf_c};
// other -> {leaf_d}.
struct PudRefutationPropagationIntegrationTest : public ::testing::Test {
    pud_leaf_nodes leaves;
    pud_refuted_nodes refuted_nodes;
    pud_children children;
    pud_node_parents parents;
    pud_root_nodes root_nodes;
    NiceMock<MockDescend> descend;
    NiceMock<MockCallSite> call_sites;
    factory_t factory{leaves, refuted_nodes, children, descend, call_sites};
    forker_t forker;
    mhws_t searches{factory, forker};
    recorder_t recorder{refuted_nodes, leaves};
    propagator_t propagator{refuted_nodes, recorder, children, parents, root_nodes};
    pud_node_id root = 1;
    pud_node_id refuted_root = 2;
    pud_node_id other = 3;
    pud_node_id leaf_a = 4;
    pud_node_id middle = 5;
    pud_node_id leaf_b = 6;
    pud_node_id leaf_c = 7;
    pud_node_id leaf_d = 8;

    void link(pud_node_id parent, std::vector<pud_node_id> node_children) {
        for (pud_node_id child : node_children)
            parents.store(child, parent);
        children.store(parent, std::move(node_children));
    }

    void SetUp() override {
        ON_CALL(descend, descend(_, _)).WillByDefault([](descent_t, pud_node_id child) {
            return std::optional<descent_t>{descent_t{child}};
        });
        root_nodes.mark_root(root);
        link(root, {refuted_root, other});
        link(refuted_root, {leaf_a, middle});
        link(middle, {leaf_b, leaf_c});
        link(other, {leaf_d});
        leaves.set_leaf(leaf_a);
        leaves.set_leaf(leaf_b);
        leaves.set_leaf(leaf_c);
        leaves.set_leaf(leaf_d);
    }

    std::vector<pud_mhws_head_id> refute_and_invalidate(pud_node_id node) {
        propagator.refute(node);
        std::vector<pud_mhws_head_id> exhausted;
        for (pud_node_id leaf : recorder.take_refuted_leaves()) {
            const std::vector<pud_mhws_head_id> leaf_exhausted = searches.invalidate_leaf(leaf);
            exhausted.insert(exhausted.end(), leaf_exhausted.begin(), leaf_exhausted.end());
        }
        return exhausted;
    }
};

// ---------- marking ----------

TEST_F(PudRefutationPropagationIntegrationTest, RefutingMarksTheWholeSubtreeAndNothingElse) {
    propagator.refute(refuted_root);
    EXPECT_TRUE(refuted_nodes.check_refuted(refuted_root));
    EXPECT_TRUE(refuted_nodes.check_refuted(leaf_a));
    EXPECT_TRUE(refuted_nodes.check_refuted(middle));
    EXPECT_TRUE(refuted_nodes.check_refuted(leaf_b));
    EXPECT_TRUE(refuted_nodes.check_refuted(leaf_c));
    EXPECT_FALSE(refuted_nodes.check_refuted(root));
    EXPECT_FALSE(refuted_nodes.check_refuted(other));
    EXPECT_FALSE(refuted_nodes.check_refuted(leaf_d));
}

TEST_F(PudRefutationPropagationIntegrationTest, RecorderCollectsTheSubtreesLeaves) {
    propagator.refute(refuted_root);
    EXPECT_THAT(recorder.take_refuted_leaves(), UnorderedElementsAre(leaf_a, leaf_b, leaf_c));
}

TEST_F(PudRefutationPropagationIntegrationTest, RefutingTwiceRecordsNothingTheSecondTime) {
    propagator.refute(refuted_root);
    recorder.take_refuted_leaves();
    propagator.refute(refuted_root);
    EXPECT_THAT(recorder.take_refuted_leaves(), IsEmpty());
}

TEST_F(PudRefutationPropagationIntegrationTest, LastLiveLeafRefutesItsAncestorsUpToALiveSibling) {
    propagator.refute(leaf_b);
    EXPECT_FALSE(refuted_nodes.check_refuted(middle));
    propagator.refute(leaf_c);
    EXPECT_TRUE(refuted_nodes.check_refuted(middle));
    EXPECT_FALSE(refuted_nodes.check_refuted(refuted_root));
    propagator.refute(leaf_a);
    EXPECT_TRUE(refuted_nodes.check_refuted(refuted_root));
    EXPECT_FALSE(refuted_nodes.check_refuted(root));
}

TEST_F(PudRefutationPropagationIntegrationTest, RefutingEverySubtreeRefutesTheRoot) {
    propagator.refute(refuted_root);
    propagator.refute(leaf_d);
    EXPECT_TRUE(refuted_nodes.check_refuted(other));
    EXPECT_TRUE(refuted_nodes.check_refuted(root));
}

TEST_F(PudRefutationPropagationIntegrationTest, AncestorsRefutedByClimbingAreNotRecordedAsLeaves) {
    propagator.refute(leaf_d);
    EXPECT_THAT(recorder.take_refuted_leaves(), UnorderedElementsAre(leaf_d));
    EXPECT_TRUE(refuted_nodes.check_refuted(other));
}

// ---------- witness searches ----------

TEST_F(PudRefutationPropagationIntegrationTest, SearchesRootedInsideTheSubtreeRunOut) {
    const pud_mhws_head_id at_refuted_root = searches.try_add_head(descent_t{refuted_root}).value();
    const pud_mhws_head_id at_middle = searches.try_add_head(descent_t{middle}).value();
    EXPECT_THAT(refute_and_invalidate(refuted_root), UnorderedElementsAre(at_refuted_root, at_middle));
}

TEST_F(PudRefutationPropagationIntegrationTest, SearchAboveTheSubtreeMovesToALiveLeafOutsideIt) {
    const pud_mhws_head_id at_root = searches.try_add_head(descent_t{root}).value();
    EXPECT_THAT(refute_and_invalidate(refuted_root), IsEmpty());
    EXPECT_THAT(refute_and_invalidate(leaf_d), UnorderedElementsAre(at_root));
}

TEST_F(PudRefutationPropagationIntegrationTest, SearchOutsideTheSubtreeIsUntouched) {
    const pud_mhws_head_id at_other = searches.try_add_head(descent_t{other}).value();
    EXPECT_THAT(refute_and_invalidate(refuted_root), IsEmpty());
    EXPECT_THAT(refute_and_invalidate(leaf_d), UnorderedElementsAre(at_other));
}

TEST_F(PudRefutationPropagationIntegrationTest, NewSearchInsideTheRefutedSubtreeFindsNothing) {
    refute_and_invalidate(refuted_root);
    EXPECT_FALSE(searches.try_add_head(descent_t{refuted_root}).has_value());
    EXPECT_FALSE(searches.try_add_head(descent_t{middle}).has_value());
    EXPECT_FALSE(searches.try_add_head(descent_t{leaf_b}).has_value());
}

TEST_F(PudRefutationPropagationIntegrationTest, NewSearchAboveTheRefutedSubtreeFindsTheLiveLeaf) {
    refute_and_invalidate(refuted_root);
    const pud_mhws_head_id at_root = searches.try_add_head(descent_t{root}).value();
    EXPECT_THAT(refute_and_invalidate(leaf_d), UnorderedElementsAre(at_root));
}

TEST_F(PudRefutationPropagationIntegrationTest, ForkedSearchSkipsTheRefutedSubtree) {
    const pud_mhws_head_id at_root = searches.try_add_head(descent_t{root}).value();
    propagator.refute(refuted_root);
    recorder.take_refuted_leaves();
    const pud_mhws_head_id forked = searches.try_fork_head(at_root, descent_t{root}).value();
    EXPECT_THAT(refute_and_invalidate(leaf_d), UnorderedElementsAre(forked));
}

TEST_F(PudRefutationPropagationIntegrationTest, ClimbingToTheRootExhaustsEverySearch) {
    const pud_mhws_head_id at_root = searches.try_add_head(descent_t{root}).value();
    const pud_mhws_head_id at_middle = searches.try_add_head(descent_t{middle}).value();
    const pud_mhws_head_id at_other = searches.try_add_head(descent_t{other}).value();
    EXPECT_THAT(refute_and_invalidate(refuted_root), UnorderedElementsAre(at_middle));
    EXPECT_THAT(refute_and_invalidate(leaf_d), UnorderedElementsAre(at_root, at_other));
    EXPECT_TRUE(refuted_nodes.check_refuted(root));
}
