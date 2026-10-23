#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_data.hpp"
#include "value_objects/expr.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace {

struct PudDataTest : public ::testing::Test {
    expr head{expr::functor{1, {}}};
    expr goal{expr::functor{2, {}}};
    expr value{expr::functor{3, {}}};
    rule first_axiom{&head, {&goal}, 2};
    rule second_axiom{&goal, {}, 0};
    pud_data data{42, {5, 9}, {first_axiom, second_axiom}};

    pud_node_delta inference_of(pud_node_id parent, pud_body_goal_id call_site) {
        return pud_node_delta{
            .parent = parent,
            .call_site = call_site,
            .substitutions = {pud_substitution{.var_idx = 0, .value = &value}},
            .body_goals = {&goal},
            .var_count = 1,
        };
    }
};

}

TEST_F(PudDataTest, AModuleKnowsItsIdAndItsDirectDependencies) {
    EXPECT_EQ(data.get_module_id(), 42u);
    EXPECT_THAT(data.get_dependencies(), ElementsAre(5u, 9u));
}

TEST_F(PudDataTest, TheAxiomsAreTheFirstNodesInTheOrderGiven) {
    EXPECT_EQ(data.count_axioms(), 2u);
    EXPECT_EQ(data.get_axiom(0), first_axiom);
    EXPECT_EQ(data.get_axiom(1), second_axiom);
}

TEST_F(PudDataTest, InferencesAreNumberedAfterTheAxiomsInTheOrderTheyAreStored) {
    EXPECT_EQ(data.store_inference(inference_of(0, 0)), 2u);
    EXPECT_EQ(data.store_inference(inference_of(1, 0)), 3u);
}

TEST_F(PudDataTest, AModuleCanHaveNoAxioms) {
    pud_data empty{1, {}, {}};
    EXPECT_EQ(empty.count_axioms(), 0u);
}

TEST_F(PudDataTest, AnAxiomHasNoParent) {
    EXPECT_EQ(data.get_parent(0), std::nullopt);
    EXPECT_EQ(data.get_parent(1), std::nullopt);
}

TEST_F(PudDataTest, AnInferenceIsStoredAsItsDeltaUnderItsParent) {
    const pud_node_id child = data.store_inference(inference_of(1, 0));
    EXPECT_EQ(data.get_inference(child), inference_of(1, 0));
    EXPECT_EQ(data.get_parent(child), std::optional<pud_node_id>{1});
}

TEST_F(PudDataTest, ANodeIsALeafUntilItsFirstChildIsStored) {
    EXPECT_TRUE(data.check_leaf(0));
    EXPECT_THAT(data.get_children(0), IsEmpty());
    const pud_node_id child = data.store_inference(inference_of(0, 0));
    EXPECT_FALSE(data.check_leaf(0));
    EXPECT_TRUE(data.check_leaf(child));
}

TEST_F(PudDataTest, ChildrenAreKeptInTheOrderTheyAreStored) {
    const pud_node_id first = data.store_inference(inference_of(0, 0));
    const pud_node_id second = data.store_inference(inference_of(0, 0));
    const pud_node_id grandchild = data.store_inference(inference_of(first, 0));
    EXPECT_THAT(data.get_children(0), ElementsAre(first, second));
    EXPECT_THAT(data.get_children(first), ElementsAre(grandchild));
}

TEST_F(PudDataTest, NoNodeStartsRefuted) {
    const pud_node_id child = data.store_inference(inference_of(0, 0));
    EXPECT_FALSE(data.check_refuted(0));
    EXPECT_FALSE(data.check_refuted(1));
    EXPECT_FALSE(data.check_refuted(child));
}

TEST_F(PudDataTest, ANodeMarkedRefutedStaysRefutedAndOnlyItIs) {
    const pud_node_id child = data.store_inference(inference_of(0, 0));
    const pud_node_id sibling = data.store_inference(inference_of(0, 0));
    data.mark_refuted(child);
    data.store_inference(inference_of(sibling, 0));
    EXPECT_TRUE(data.check_refuted(child));
    EXPECT_FALSE(data.check_refuted(sibling));
    EXPECT_FALSE(data.check_refuted(0));
}

TEST_F(PudDataTest, AnAxiomCanBeRefuted) {
    data.mark_refuted(1);
    EXPECT_TRUE(data.check_refuted(1));
    EXPECT_FALSE(data.check_refuted(0));
}

TEST_F(PudDataTest, StoredNodesStayInPlaceWhileLaterNodesAreStored) {
    const pud_node_id child = data.store_inference(inference_of(0, 0));
    const rule& stored_axiom = data.get_axiom(0);
    const pud_node_delta& stored_inference = data.get_inference(child);
    const std::vector<pud_node_id>& children = data.get_children(0);
    for (int stored = 0; stored < 1000; ++stored)
        data.store_inference(inference_of(1, 0));
    EXPECT_EQ(&stored_axiom, &data.get_axiom(0));
    EXPECT_EQ(&stored_inference, &data.get_inference(child));
    EXPECT_EQ(&children, &data.get_children(0));
}
