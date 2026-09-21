// Integration: witness search + candidate search on the per-node tables.

#include <gtest/gtest.h>
#include <variant>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/fully_persistent_array.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/order_maintenance.hpp"
#include "infrastructure/pud_candidate_search.hpp"
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "infrastructure/pud_node_added_unifications.hpp"
#include "infrastructure/pud_node_base_interval.hpp"
#include "infrastructure/pud_node_children.hpp"
#include "infrastructure/pud_node_lvc.hpp"
#include "infrastructure/pud_rule_id_pool.hpp"
#include "infrastructure/pud_unify_head.hpp"
#include "infrastructure/pud_witness_search.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"

using unify_head_t = pud_unify_head<
    order_maintenance, pud_node_children, pud_node_added_unifications,
    fully_persistent_array, fully_persistent_array,
    globalizer, expr_pool, expr_pool>;
using witness_search_t = pud_witness_search<
    pud_node_children, pud_node_children, pud_node_children, unify_head_t>;
using candidate_search_t = pud_candidate_search<
    witness_search_t, pud_node_children, pud_node_children, pud_node_children,
    unify_head_t, pud_node_added_body_goals>;

struct PudForestSearchIntegrationTest : public ::testing::Test {
    PudForestSearchIntegrationTest()
        : unify_head_(om_, children_, added_unifications_, fpa_, fpa_, glob_, exprs_, exprs_)
        , witness_(children_, children_, children_, unify_head_)
        , candidate_(witness_, children_, children_, children_, unify_head_,
                     added_body_goals_) {}

    const pud_rule_id* add_axiom(size_t entry_idx,
                                 std::vector<pud_added_unification> unifs,
                                 std::vector<const expr*> body,
                                 uint32_t lvc) {
        const pud_rule_id* id = pool_.make_axiom(entry_idx);
        added_unifications_.store(id, std::move(unifs));
        added_body_goals_.store(id, std::move(body));
        lvc_.store(id, lvc);
        children_.add_root(id);
        base_interval_.store(id, om_.allocate_root());
        return id;
    }

    const pud_rule_id* add_inference(const pud_rule_id* caller,
                                     size_t call_site,
                                     const pud_rule_id* callee,
                                     std::vector<pud_added_unification> unifs,
                                     std::vector<const expr*> body,
                                     uint32_t lvc) {
        const pud_rule_id* id = pool_.make_inference(caller, call_site, callee);
        added_unifications_.store(id, std::move(unifs));
        added_body_goals_.store(id, std::move(body));
        lvc_.store(id, lvc);
        return id;
    }

    pud_rule_id_pool pool_;
    order_maintenance om_;
    fully_persistent_array fpa_;
    globalizer glob_;
    expr_pool exprs_;
    pud_node_added_unifications added_unifications_;
    pud_node_added_body_goals added_body_goals_;
    pud_node_lvc lvc_;
    pud_node_children children_;
    pud_node_base_interval base_interval_;
    unify_head_t unify_head_;
    witness_search_t witness_;
    candidate_search_t candidate_;
};

TEST_F(PudForestSearchIntegrationTest, WitnessSearchFindsUnifyingAxiomLeaf) {
    const expr* pred = exprs_.make_functor(4, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, pred}}, {}, 1);
    pud_query query{
        om_.allocate_child_of(base_interval_.get(axiom)),
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
    const pud_rule_id* axiom = add_axiom(0, {{0, pred}}, {}, 1);
    pud_query query{
        om_.allocate_child_of(base_interval_.get(axiom)),
        pred,
        {},
        1};
    pud_candidate_search_context ctx{axiom, {}, added_body_goals_.get(axiom)};
    const pud_candidate_search_result result = candidate_.resume(query, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
}

TEST_F(PudForestSearchIntegrationTest, CandidateSearchRefutesAxiomWhenHeadDoesNotUnify) {
    const expr* head = exprs_.make_functor(6, {});
    const expr* body = exprs_.make_functor(7, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, head}}, {body}, 1);
    pud_query query{
        om_.allocate_child_of(base_interval_.get(axiom)),
        body,
        {},
        1};
    pud_candidate_search_context ctx{axiom, {}, added_body_goals_.get(axiom)};
    const pud_candidate_search_result result = candidate_.resume(query, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::axiom_refuted>(result.content));
}

TEST_F(PudForestSearchIntegrationTest, WitnessSearchFindsGrandchildUnderLinkedForest) {
    const expr* pred = exprs_.make_functor(8, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, pred}}, {pred}, 1);
    const pud_rule_id* child = add_inference(axiom, 0, axiom, {{0, pred}}, {pred}, 1);
    children_.link_children(axiom, {child});
    base_interval_.store(child, om_.allocate_child_of(base_interval_.get(axiom)));
    const pud_rule_id* grand = add_inference(child, 0, axiom, {{0, pred}}, {}, 1);
    children_.link_children(child, {grand});
    base_interval_.store(grand, om_.allocate_child_of(base_interval_.get(child)));
    pud_query query{
        om_.allocate_child_of(base_interval_.get(grand)),
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
    const pud_rule_id* axiom = add_axiom(0, {{0, pred}}, {pred}, 1);
    const pud_rule_id* c0 = add_inference(axiom, 0, axiom, {{0, pred}}, {}, 1);
    const pud_rule_id* c1 = add_inference(axiom, 1, axiom, {{0, pred}}, {}, 1);
    children_.link_children(axiom, {c0, c1});
    base_interval_.store(c0, om_.allocate_child_of(base_interval_.get(axiom)));
    base_interval_.store(c1, om_.allocate_child_of(base_interval_.get(axiom)));
    pud_query query{
        om_.allocate_child_of(base_interval_.get(axiom)),
        pred,
        {},
        1};
    pud_candidate_search_context ctx{axiom, {}, added_body_goals_.get(axiom)};
    const pud_candidate_search_result result = candidate_.resume(query, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::choice_point>(result.content));
    EXPECT_EQ(ctx.live_edges.size(), 2u);
}

TEST_F(PudForestSearchIntegrationTest, CandidateSearchAdvancesWhenWitnessCurrentIsDeeper) {
    const expr* pred = exprs_.make_functor(10, {});
    const pud_rule_id* axiom = add_axiom(0, {{0, pred}}, {pred}, 1);
    const pud_rule_id* child = add_inference(axiom, 0, axiom, {{0, pred}}, {pred}, 1);
    children_.link_children(axiom, {child});
    base_interval_.store(child, om_.allocate_child_of(base_interval_.get(axiom)));
    const pud_rule_id* grand = add_inference(child, 0, axiom, {{0, pred}}, {}, 1);
    children_.link_children(child, {grand});
    base_interval_.store(grand, om_.allocate_child_of(base_interval_.get(child)));
    pud_query query{
        om_.allocate_child_of(base_interval_.get(axiom)),
        pred,
        {},
        1};
    pud_candidate_search_context ctx{axiom, {}, added_body_goals_.get(axiom)};
    const pud_candidate_search_result result = candidate_.resume(query, ctx);
    EXPECT_TRUE(std::holds_alternative<pud_candidate_search_result::self_witness>(result.content));
    EXPECT_EQ(ctx.cursor, grand);
}
