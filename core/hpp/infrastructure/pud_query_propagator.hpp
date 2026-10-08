#ifndef PUD_QUERY_PROPAGATOR_HPP
#define PUD_QUERY_PROPAGATOR_HPP

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include <immer/set.hpp>
#include <immer/set_transient.hpp>
#include "infrastructure/coroutine.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_query_node.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

template<
    typename BindMap,
    typename Unifier,
    typename Specializer,
    typename Normalizer,
    typename IGetNodeParent,
    typename IMakeNode,
    typename IMakeVar,
    typename IGlobalize,
    typename ICheckNodeRefuted,
    typename IGetCallSite,
    typename IIterateRoots>
struct pud_query_propagator {
    struct query_node_handle {
        const pud_node* node() const;
    private:
        query_node_handle(std::shared_ptr<pud_query_node> node);
        std::shared_ptr<pud_query_node> query_node;
        friend struct pud_query_propagator;
    };
    pud_query_propagator(
        IGetNodeParent& get_node_parent,
        IMakeNode& make_node,
        IMakeVar& make_var,
        IGlobalize& globalize,
        ICheckNodeRefuted& check_node_refuted,
        IGetCallSite& get_call_site,
        IIterateRoots& iterate_roots);
    std::vector<query_node_handle> roots();
    std::optional<query_node_handle> propagate(query_node_handle current, const pud_node* child_node);
    std::vector<query_node_handle> open_query(query_node_handle caller, const expr* query);
    const pud_node* close_query(query_node_handle query);
private:
    IGetNodeParent& get_node_parent_;
    IMakeNode& make_node_;
    IMakeVar& make_var_;
    IGlobalize& globalize_;
    ICheckNodeRefuted& check_node_refuted_;
    IGetCallSite& get_call_site_;
    IIterateRoots& iterate_roots_;
};

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR,
         typename IGCS, typename IIR>
pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR, IGCS, IIR>::query_node_handle::query_node_handle(
    std::shared_ptr<pud_query_node> node)
    : query_node(std::move(node)) {}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR,
         typename IGCS, typename IIR>
const pud_node* pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR, IGCS, IIR>::query_node_handle::node() const {
    return query_node->node;
}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR,
         typename IGCS, typename IIR>
pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR, IGCS, IIR>::pud_query_propagator(
    IGNP& get_node_parent,
    IMN& make_node,
    IMV& make_var,
    IG& globalize,
    IGNR& check_node_refuted,
    IGCS& get_call_site,
    IIR& iterate_roots)
    : get_node_parent_(get_node_parent)
    , make_node_(make_node)
    , make_var_(make_var)
    , globalize_(globalize)
    , check_node_refuted_(check_node_refuted)
    , get_call_site_(get_call_site)
    , iterate_roots_(iterate_roots) {}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR,
         typename IGCS, typename IIR>
std::vector<typename pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR, IGCS, IIR>::query_node_handle>
pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR, IGCS, IIR>::roots() {
    std::vector<query_node_handle> handles;
    auto co = iterate_roots_.iterate_roots();
    while (auto opt = co.next()) {
        const pud_node* r = opt.value();
        auto body_goals_transient = immer::map<body_goal_id, const expr*>{}.transient();
        size_t bgc = 0;
        for (const expr* goal : r->added_body_goals)
            body_goals_transient.set(bgc++, goal);
        handles.push_back(query_node_handle{
            std::make_shared<pud_query_node>(
                pud_query_node{
                    .frame_offset        = 0,
                    .node                = r,
                    .touched_caller_reps = {},
                    .bindings            = {},
                    .lvc                 = 1 + r->added_var_count,
                    .pending_body_goals  = std::move(body_goals_transient).persistent(),
                    .bgc                 = bgc
                })});
    }
    return handles;
}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR,
         typename IGCS, typename IIR>
std::optional<typename pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR, IGCS, IIR>::query_node_handle>
pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR, IGCS, IIR>::propagate(query_node_handle current, const pud_node* child_node) {
    DEBUG_ASSERT(get_node_parent_.get(child_node) == current.query_node->node);

    if (check_node_refuted_.check_refuted(child_node))
        return std::nullopt;

    auto transient = current.query_node->bindings.transient();
    BM bind_map{globalize_, transient};
    U unifier{globalize_, &bind_map};
    S specializer{make_var_, unifier};

    uint32_t frame_offset = current.query_node->frame_offset;

    auto touched_transient = current.query_node->touched_caller_reps.transient();
    for (pud_specialization spec : child_node->added_specializations) {
        auto sm = specializer.specialize(frame_offset, spec);
        while (auto rep = sm.next())
            touched_transient.insert(*rep);
        if (!sm.result())
            return std::nullopt;
    }
    immer::set<uint32_t> new_touched = std::move(touched_transient).persistent();

    immer::map<uint32_t, framed_expr> child_bindings = std::move(transient).persistent();

    auto body_goals_transient = current.query_node->pending_body_goals.transient();
    body_goals_transient.erase(get_call_site_.get(current.query_node->node));
    size_t child_bgc = current.query_node->bgc;
    for (const expr* goal : child_node->added_body_goals)
        body_goals_transient.set(child_bgc++, goal);
    immer::map<body_goal_id, const expr*> child_body_goals = std::move(body_goals_transient).persistent();

    uint32_t lvc = current.query_node->lvc;

    return query_node_handle{
        std::make_shared<pud_query_node>(
            pud_query_node{
                .frame_offset        = frame_offset,
                .node                = child_node,
                .touched_caller_reps = std::move(new_touched),
                .bindings            = std::move(child_bindings),
                .lvc                 = lvc + child_node->added_var_count,
                .pending_body_goals  = std::move(child_body_goals),
                .bgc                 = child_bgc
            })};
}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR,
         typename IGCS, typename IIR>
std::vector<typename pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR, IGCS, IIR>::query_node_handle>
pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR, IGCS, IIR>::open_query(query_node_handle caller, const expr* query_expr) {
    uint32_t caller_frame_offset = caller.query_node->frame_offset;
    uint32_t caller_lvc = caller.query_node->lvc;

    uint32_t query_frame_offset = caller_frame_offset + caller_lvc;

    auto transient = caller.query_node->bindings.transient();
    BM bind_map{globalize_, transient};

    uint32_t query_head_var = globalize_.globalize(query_frame_offset, 0);

    bind_map.bind(
        query_head_var,
        framed_expr{
            query_expr,
            caller_frame_offset});

    immer::map<uint32_t, framed_expr> query_bindings = std::move(transient).persistent();

    std::vector<query_node_handle> handles;
    auto co = iterate_roots_.iterate_roots();
    while (auto opt = co.next()) {
        const pud_node* r = opt.value();
        auto body_goals_transient = immer::map<body_goal_id, const expr*>{}.transient();
        size_t bgc = 0;
        for (const expr* goal : r->added_body_goals)
            body_goals_transient.set(bgc++, goal);
        handles.push_back(query_node_handle{
            std::make_shared<pud_query_node>(
                pud_query_node{
                    .frame_offset        = query_frame_offset,
                    .node                = r,
                    .touched_caller_reps = {},
                    .bindings            = query_bindings,
                    .lvc                 = r->added_var_count,
                    .pending_body_goals  = std::move(body_goals_transient).persistent(),
                    .bgc                 = bgc
                })});
    }
    return handles;
}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR,
         typename IGCS, typename IIR>
const pud_node* pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR, IGCS, IIR>::close_query(query_node_handle current) {
    uint32_t frame_offset = current.query_node->frame_offset;

    auto transient = current.query_node->bindings.transient();
    BM bind_map{globalize_, transient};
    N normalizer{globalize_, make_var_, make_var_, bind_map};

    std::unordered_map<uint32_t, uint32_t> translation_map;

    std::vector<pud_specialization> added_specializations;
    std::vector<const expr*> added_body_goals;

    for (uint32_t rep : current.query_node->touched_caller_reps) {
        framed_expr value = bind_map.whnf({make_var_.make_var(rep), 0});
        added_specializations.push_back(pud_specialization{
            .var_idx = rep,
            .value   = normalizer.normalize(value, frame_offset, translation_map)
        });
    }

    for (auto const& [id, goal] : current.query_node->pending_body_goals) {
        framed_expr goal_framed{goal, frame_offset};
        added_body_goals.push_back(
            normalizer.normalize(goal_framed, frame_offset, translation_map));
    }

    uint32_t added_var_count = translation_map.size();

    return make_node_.make(
        added_specializations,
        added_body_goals,
        added_var_count
    );
}

#endif
