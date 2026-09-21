// Integration: witness search + candidate search on a real PUD forest.

#include <gtest/gtest.h>
#include <variant>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/fully_persistent_array.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/order_maintenance.hpp"
#include "infrastructure/pud_forest.hpp"
#include "infrastructure/pud_candidate_search.hpp"
#include "infrastructure/pud_rule_id_pool.hpp"
#include "infrastructure/pud_unify_head.hpp"
#include "infrastructure/pud_witness_search.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"

using forest_t = pud_forest<pud_rule_id_pool, pud_rule_id_pool, order_maintenance, order_maintenance>;
using unify_head_t = pud_unify_head<
    order_maintenance, forest_t, forest_t,
    fully_persistent_array, fully_persistent_array,
    globalizer, expr_pool, expr_pool>;
using witness_search_t = pud_witness_search<forest_t, forest_t, forest_t, unify_head_t>;
using candidate_search_t = pud_candidate_search<
    witness_search_t, forest_t, forest_t, forest_t, unify_head_t>;

struct PudForestSearchIntegrationTest : public ::testing::Test {
    PudForestSearchIntegrationTest()
        : forest_(pool_, pool_, om_, om_)
        , unify_head_(om_, forest_, forest_, fpa_, fpa_, glob_, exprs_, exprs_)
        , witness_(forest_, forest_, forest_, unify_head_)
        , candidate_(witness_, forest_, forest_, forest_, unify_head_) {}

    pud_rule_id_pool pool_;
    order_maintenance om_;
    fully_persistent_array fpa_;
    globalizer glob_;
    expr_pool exprs_;
    forest_t forest_;
    unify_head_t unify_head_;
    witness_search_t witness_;
    candidate_search_t candidate_;
};

TEST_F(PudForestSearchIntegrationTest, WitnessSearchFindsUnifyingAxiomLeaf) {
    const expr* pred = exprs_.make_functor(4, {});
    const pud_rule_id* axiom = forest_.add_axiom(0, {{0, pred}}, {}, 1);
    pud_query query{
        om_.allocate_child_of(forest_.get_node(axiom).interval),
        pred,
        {},
        1};
    pud_witness_search_context ctx{axiom, axiom};
    const pud_witness_search_result result = witness_.resume(query, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
    EXPECT_EQ(ctx.current, axiom);
}

TEST_F(PudForestSearchIntegrationTest, CandidateSearchSelfWitnessesMatchingLeaf) {
    const expr* pred = exprs_.make_functor(5, {});
    const pud_rule_id* axiom = forest_.add_axiom(0, {{0, pred}}, {}, 1);
    pud_query query{
        om_.allocate_child_of(forest_.get_node(axiom).interval),
        pred,
        {},
        1};
    pud_candidate_search_context ctx{axiom, {}};
    const pud_candidate_search_result result = candidate_.resume(query, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
}

TEST_F(PudForestSearchIntegrationTest, CandidateSearchRefutesAxiomWhenHeadDoesNotUnify) {
    const expr* head = exprs_.make_functor(6, {});
    const expr* body = exprs_.make_functor(7, {});
    const pud_rule_id* axiom = forest_.add_axiom(0, {{0, head}}, {body}, 1);
    pud_query query{
        om_.allocate_child_of(forest_.get_node(axiom).interval),
        body,
        {},
        1};
    pud_candidate_search_context ctx{axiom, {}};
    const pud_candidate_search_result result = candidate_.resume(query, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::axiom_refuted>(result.content));
}

TEST_F(PudForestSearchIntegrationTest, WitnessSearchFindsGrandchildUnderLinkedForest) {
    const expr* pred = exprs_.make_functor(8, {});
    const pud_rule_id* axiom = forest_.add_axiom(0, {{0, pred}}, {pred}, 1);
    const pud_rule_id* child = forest_.add_inference(axiom, 0, axiom, {{0, pred}}, {pred}, 1);
    forest_.link_children(axiom, {child});
    const pud_rule_id* grand = forest_.add_inference(child, 0, axiom, {{0, pred}}, {}, 1);
    forest_.link_children(child, {grand});
    pud_query query{
        om_.allocate_child_of(forest_.get_node(grand).interval),
        pred,
        {},
        1};
    pud_witness_search_context ctx{axiom, axiom};
    const pud_witness_search_result result = witness_.resume(query, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_witness_search_result::found>(result.content));
    EXPECT_EQ(ctx.current, grand);
}

TEST_F(PudForestSearchIntegrationTest, CandidateSearchChoicePointOnTwoLinkedChildren) {
    const expr* pred = exprs_.make_functor(9, {});
    const pud_rule_id* axiom = forest_.add_axiom(0, {{0, pred}}, {pred}, 1);
    const pud_rule_id* c0 = forest_.add_inference(axiom, 0, axiom, {{0, pred}}, {}, 1);
    const pud_rule_id* c1 = forest_.add_inference(axiom, 1, axiom, {{0, pred}}, {}, 1);
    forest_.link_children(axiom, {c0, c1});
    pud_query query{
        om_.allocate_child_of(forest_.get_node(axiom).interval),
        pred,
        {},
        1};
    pud_candidate_search_context ctx{axiom, {}};
    const pud_candidate_search_result result = candidate_.resume(query, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::choice_point>(result.content));
    EXPECT_EQ(ctx.live_edges.size(), 2u);
}

TEST_F(PudForestSearchIntegrationTest, CandidateSearchAdvancesWhenWitnessCurrentIsDeeper) {
    const expr* pred = exprs_.make_functor(10, {});
    const pud_rule_id* axiom = forest_.add_axiom(0, {{0, pred}}, {pred}, 1);
    const pud_rule_id* child = forest_.add_inference(axiom, 0, axiom, {{0, pred}}, {pred}, 1);
    forest_.link_children(axiom, {child});
    const pud_rule_id* grand = forest_.add_inference(child, 0, axiom, {{0, pred}}, {}, 1);
    forest_.link_children(child, {grand});
    pud_query query{
        om_.allocate_child_of(forest_.get_node(axiom).interval),
        pred,
        {},
        1};
    pud_candidate_search_context ctx{axiom, {}};
    const pud_candidate_search_result result = candidate_.resume(query, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
    EXPECT_EQ(ctx.cursor, grand);
}
