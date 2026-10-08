#ifndef PUD_QUERY_PROPAGATOR_HPP
#define PUD_QUERY_PROPAGATOR_HPP

#include <optional>
#include <unordered_map>
#include <vector>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include <immer/set.hpp>
#include <immer/set_transient.hpp>
#include "infrastructure/coroutine.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_query_handle.hpp"
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
    pud_query_propagator(
        IGetNodeParent& get_node_parent,
        IMakeNode& make_node,
        IMakeVar& make_var,
        IGlobalize& globalize,
        ICheckNodeRefuted& check_node_refuted,
        IGetCallSite& get_call_site,
        IIterateRoots& iterate_roots);
    std::vector<pud_query_handle> roots();
    std::optional<pud_query_handle> propagate(pud_query_handle current, const pud_node* child_node);
    std::vector<pud_query_handle> open_query(pud_query_handle caller, const expr* query);
    const pud_node* close_query(pud_query_handle query);
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
std::vector<pud_query_handle>
pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR, IGCS, IIR>::roots() {
    std::vector<pud_query_handle> handles;
    auto co = iterate_roots_.iterate_roots();
    while (auto opt = co.next()) {
        const pud_node* r = opt.value();
        auto bindings_transient = immer::map<uint32_t, framed_expr>{}.transient();
        BM bind_map{globalize_, bindings_transient};
        for (pud_specialization spec : r->added_specializations)
            bind_map.bind(globalize_.globalize(0, spec.var_idx), framed_expr{spec.value, 0});
        auto body_goals_transient = immer::map<body_goal_id, const expr*>{}.transient();
        size_t bgc = 0;
        for (const expr* goal : r->added_body_goals)
            body_goals_transient.set(bgc++, goal);
        handles.push_back(pud_query_handle{
            .frame_offset        = 0,
            .lvc                 = 1 + r->added_var_count,
            .bgc                 = bgc,
            .node                = r,
            .touched_caller_reps = {},
            .bindings            = std::move(bindings_transient).persistent(),
            .pending_body_goals  = std::move(body_goals_transient).persistent()
        });
    }
    return handles;
}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR,
         typename IGCS, typename IIR>
std::optional<pud_query_handle>
pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR, IGCS, IIR>::propagate(pud_query_handle current, const pud_node* child_node) {
    DEBUG_ASSERT(get_node_parent_.get(child_node) == current.node);

    if (check_node_refuted_.check_refuted(child_node))
        return std::nullopt;

    auto transient = current.bindings.transient();
    BM bind_map{globalize_, transient};
    U unifier{globalize_, &bind_map};
    S specializer{make_var_, unifier};

    auto touched_transient = current.touched_caller_reps.transient();
    for (pud_specialization spec : child_node->added_specializations) {
        auto sm = specializer.specialize(current.frame_offset, spec);
        while (auto rep = sm.next())
            touched_transient.insert(*rep);
        if (!sm.result())
            return std::nullopt;
    }

    auto body_goals_transient = current.pending_body_goals.transient();
    body_goals_transient.erase(get_call_site_.get(current.node));
    size_t child_bgc = current.bgc;
    for (const expr* goal : child_node->added_body_goals)
        body_goals_transient.set(child_bgc++, goal);

    return pud_query_handle{
        .frame_offset        = current.frame_offset,
        .lvc                 = current.lvc + child_node->added_var_count,
        .bgc                 = child_bgc,
        .node                = child_node,
        .touched_caller_reps = std::move(touched_transient).persistent(),
        .bindings            = std::move(transient).persistent(),
        .pending_body_goals  = std::move(body_goals_transient).persistent()
    };
}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR,
         typename IGCS, typename IIR>
std::vector<pud_query_handle>
pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR, IGCS, IIR>::open_query(pud_query_handle caller, const expr* query_expr) {
    uint32_t query_frame_offset = caller.frame_offset + caller.lvc;

    auto transient = caller.bindings.transient();
    BM bind_map{globalize_, transient};

    uint32_t query_head_var = globalize_.globalize(query_frame_offset, 0);
    bind_map.bind(
        query_head_var,
        framed_expr{query_expr, caller.frame_offset});

    immer::map<uint32_t, framed_expr> query_bindings = std::move(transient).persistent();

    std::vector<pud_query_handle> handles;
    auto co = iterate_roots_.iterate_roots();
    while (auto opt = co.next()) {
        const pud_node* r = opt.value();
        auto bindings_transient = query_bindings.transient();
        BM root_bind_map{globalize_, bindings_transient};
        for (pud_specialization spec : r->added_specializations)
            root_bind_map.bind(globalize_.globalize(query_frame_offset, spec.var_idx), framed_expr{spec.value, query_frame_offset});
        auto body_goals_transient = immer::map<body_goal_id, const expr*>{}.transient();
        size_t bgc = 0;
        for (const expr* goal : r->added_body_goals)
            body_goals_transient.set(bgc++, goal);
        handles.push_back(pud_query_handle{
            .frame_offset        = query_frame_offset,
            .lvc                 = r->added_var_count,
            .bgc                 = bgc,
            .node                = r,
            .touched_caller_reps = {},
            .bindings            = std::move(bindings_transient).persistent(),
            .pending_body_goals  = std::move(body_goals_transient).persistent()
        });
    }
    return handles;
}

template<typename BM, typename U, typename S, typename N,
         typename IGNP, typename IMN, typename IMV, typename IG, typename IGNR,
         typename IGCS, typename IIR>
const pud_node* pud_query_propagator<BM, U, S, N, IGNP, IMN, IMV, IG, IGNR, IGCS, IIR>::close_query(pud_query_handle current) {
    auto transient = current.bindings.transient();
    BM bind_map{globalize_, transient};
    N normalizer{globalize_, make_var_, make_var_, bind_map};

    std::unordered_map<uint32_t, uint32_t> translation_map;

    std::vector<pud_specialization> added_specializations;
    std::vector<const expr*> added_body_goals;

    for (uint32_t rep : current.touched_caller_reps) {
        framed_expr value = bind_map.whnf({make_var_.make_var(rep), 0});
        added_specializations.push_back(pud_specialization{
            .var_idx = rep,
            .value   = normalizer.normalize(value, current.frame_offset, translation_map)
        });
    }

    for (auto const& [id, goal] : current.pending_body_goals) {
        added_body_goals.push_back(
            normalizer.normalize({goal, current.frame_offset}, current.frame_offset, translation_map));
    }

    uint32_t added_var_count = translation_map.size();

    return make_node_.make(
        added_specializations,
        added_body_goals,
        added_var_count
    );
}

#endif
