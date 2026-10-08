#ifndef PUD_DESCENDER_HPP
#define PUD_DESCENDER_HPP

#include <optional>
#include <unordered_map>
#include <vector>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include <immer/set.hpp>
#include <immer/set_transient.hpp>
#include "infrastructure/coroutine.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_descent.hpp"
#include "value_objects/pud_specialization.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

template<
    typename BindMap,
    typename Unifier,
    typename Specializer,
    typename Normalizer,
    typename INextNodeId,
    typename IMakeVar,
    typename IGlobalize,
    typename ICheckNodeRefuted,
    typename IGetCallSite,
    typename IGetAddedSpecializations,
    typename IGetAddedBodyGoals,
    typename IGetAddedVarCount,
    typename IGetAxiomHead,
    typename IStoreAddedSpecializations,
    typename IStoreAddedBodyGoals,
    typename IStoreAddedVarCount>
struct pud_descender {
    pud_descender(
        INextNodeId& next_node_id,
        IMakeVar& make_var,
        IGlobalize& globalize,
        ICheckNodeRefuted& check_node_refuted,
        IGetCallSite& get_call_site,
        IGetAddedSpecializations& get_added_specializations,
        IGetAddedBodyGoals& get_added_body_goals,
        IGetAddedVarCount& get_added_var_count,
        IGetAxiomHead& get_axiom_head,
        IStoreAddedSpecializations& store_added_specializations,
        IStoreAddedBodyGoals& store_added_body_goals,
        IStoreAddedVarCount& store_added_var_count);

    pud_descent descent_root(pud_node_id root_id);
    std::optional<pud_descent> descend(pud_descent current, pud_node_id child_node_id);
    std::optional<pud_descent> open_query(pud_descent caller, const expr* query_expr, pud_node_id root_id);
    pud_node_id close_query(pud_descent query);
private:
    INextNodeId& next_node_id_;
    IMakeVar& make_var_;
    IGlobalize& globalize_;
    ICheckNodeRefuted& check_node_refuted_;
    IGetCallSite& get_call_site_;
    IGetAddedSpecializations& get_added_specializations_;
    IGetAddedBodyGoals& get_added_body_goals_;
    IGetAddedVarCount& get_added_var_count_;
    IGetAxiomHead& get_axiom_head_;
    IStoreAddedSpecializations& store_added_specializations_;
    IStoreAddedBodyGoals& store_added_body_goals_;
    IStoreAddedVarCount& store_added_var_count_;
};

template<typename BM, typename U, typename S, typename N,
         typename INNI, typename IMV, typename IG, typename IGNR,
         typename IGCS,
         typename IGAS, typename IGABG, typename IGAVC, typename IGAH,
         typename ISAS, typename ISABG, typename ISAVC>
pud_descender<BM, U, S, N, INNI, IMV, IG, IGNR, IGCS, IGAS, IGABG, IGAVC, IGAH, ISAS, ISABG, ISAVC>::pud_descender(
    INNI& next_node_id,
    IMV& make_var,
    IG& globalize,
    IGNR& check_node_refuted,
    IGCS& get_call_site,
    IGAS& get_added_specializations,
    IGABG& get_added_body_goals,
    IGAVC& get_added_var_count,
    IGAH& get_axiom_head,
    ISAS& store_added_specializations,
    ISABG& store_added_body_goals,
    ISAVC& store_added_var_count)
    : next_node_id_(next_node_id)
    , make_var_(make_var)
    , globalize_(globalize)
    , check_node_refuted_(check_node_refuted)
    , get_call_site_(get_call_site)
    , get_added_specializations_(get_added_specializations)
    , get_added_body_goals_(get_added_body_goals)
    , get_added_var_count_(get_added_var_count)
    , get_axiom_head_(get_axiom_head)
    , store_added_specializations_(store_added_specializations)
    , store_added_body_goals_(store_added_body_goals)
    , store_added_var_count_(store_added_var_count) {}

template<typename BM, typename U, typename S, typename N,
         typename INNI, typename IMV, typename IG, typename IGNR,
         typename IGCS,
         typename IGAS, typename IGABG, typename IGAVC, typename IGAH,
         typename ISAS, typename ISABG, typename ISAVC>
pud_descent
pud_descender<BM, U, S, N, INNI, IMV, IG, IGNR, IGCS, IGAS, IGABG, IGAVC, IGAH, ISAS, ISABG, ISAVC>::descent_root(
    pud_node_id root_id) {
    const uint32_t root_var_count = get_added_var_count_.get(root_id);
    auto body_goals_transient = immer::map<body_goal_id, const expr*>{}.transient();
    size_t bgc = 0;
    for (const expr* goal : get_added_body_goals_.get(root_id))
        body_goals_transient.set(bgc++, goal);
    return pud_descent{
        .frame_offset        = 0,
        .lvc                 = root_var_count,
        .bgc                 = bgc,
        .node                = root_id,
        .touched_caller_reps = {},
        .bindings            = {},
        .pending_body_goals  = std::move(body_goals_transient).persistent()
    };
}

template<typename BM, typename U, typename S, typename N,
         typename INNI, typename IMV, typename IG, typename IGNR,
         typename IGCS,
         typename IGAS, typename IGABG, typename IGAVC, typename IGAH,
         typename ISAS, typename ISABG, typename ISAVC>
std::optional<pud_descent>
pud_descender<BM, U, S, N, INNI, IMV, IG, IGNR, IGCS, IGAS, IGABG, IGAVC, IGAH, ISAS, ISABG, ISAVC>::descend(
    pud_descent current, pud_node_id child_node_id) {

    if (check_node_refuted_.check_refuted(child_node_id))
        return std::nullopt;

    auto bindings_transient = current.bindings.transient();
    BM bind_map{globalize_, bindings_transient};
    U unifier{globalize_, &bind_map};
    S specializer{make_var_, unifier};

    auto touched_transient = current.touched_caller_reps.transient();
    for (pud_specialization spec : get_added_specializations_.get(child_node_id)) {
        auto sm = specializer.specialize(current.frame_offset, spec);
        while (auto rep = sm.next())
            touched_transient.insert(*rep);
        if (!sm.result())
            return std::nullopt;
    }

    auto body_goals_transient = current.pending_body_goals.transient();
    body_goals_transient.erase(get_call_site_.get(current.node));
    size_t child_bgc = current.bgc;
    for (const expr* goal : get_added_body_goals_.get(child_node_id))
        body_goals_transient.set(child_bgc++, goal);

    return pud_descent{
        .frame_offset        = current.frame_offset,
        .lvc                 = current.lvc + get_added_var_count_.get(child_node_id),
        .bgc                 = child_bgc,
        .node                = child_node_id,
        .touched_caller_reps = std::move(touched_transient).persistent(),
        .bindings            = std::move(bindings_transient).persistent(),
        .pending_body_goals  = std::move(body_goals_transient).persistent()
    };
}

template<typename BM, typename U, typename S, typename N,
         typename INNI, typename IMV, typename IG, typename IGNR,
         typename IGCS,
         typename IGAS, typename IGABG, typename IGAVC, typename IGAH,
         typename ISAS, typename ISABG, typename ISAVC>
std::optional<pud_descent>
pud_descender<BM, U, S, N, INNI, IMV, IG, IGNR, IGCS, IGAS, IGABG, IGAVC, IGAH, ISAS, ISABG, ISAVC>::open_query(
    pud_descent caller, const expr* query_expr, pud_node_id root_id) {

    const uint32_t query_frame_offset = caller.frame_offset + caller.lvc;
    const expr* axiom_head = get_axiom_head_.get(root_id);
    const uint32_t root_var_count = get_added_var_count_.get(root_id);

    // Introduce a fresh anchor var (just beyond the axiom's var range) that
    // links the query expression to the axiom head via the specializer.
    const uint32_t anchor_var_idx = root_var_count;
    const uint32_t anchor_var_global = globalize_.globalize(query_frame_offset, anchor_var_idx);

    auto transient = caller.bindings.transient();
    BM bind_map{globalize_, transient};
    bind_map.bind(anchor_var_global, framed_expr{query_expr, caller.frame_offset});

    U unifier{globalize_, &bind_map};
    S specializer{make_var_, unifier};

    auto touched_transient = immer::set<uint32_t>{}.transient();
    auto sm = specializer.specialize(query_frame_offset, pud_specialization{
        .var_idx = anchor_var_idx,
        .value   = axiom_head
    });
    while (auto rep = sm.next())
        touched_transient.insert(*rep);
    if (!sm.result())
        return std::nullopt;

    auto body_goals_transient = immer::map<body_goal_id, const expr*>{}.transient();
    size_t bgc = 0;
    for (const expr* goal : get_added_body_goals_.get(root_id))
        body_goals_transient.set(bgc++, goal);

    return pud_descent{
        .frame_offset        = query_frame_offset,
        .lvc                 = root_var_count + 1,
        .bgc                 = bgc,
        .node                = root_id,
        .touched_caller_reps = std::move(touched_transient).persistent(),
        .bindings            = std::move(transient).persistent(),
        .pending_body_goals  = std::move(body_goals_transient).persistent()
    };
}

template<typename BM, typename U, typename S, typename N,
         typename INNI, typename IMV, typename IG, typename IGNR,
         typename IGCS,
         typename IGAS, typename IGABG, typename IGAVC, typename IGAH,
         typename ISAS, typename ISABG, typename ISAVC>
pud_node_id
pud_descender<BM, U, S, N, INNI, IMV, IG, IGNR, IGCS, IGAS, IGABG, IGAVC, IGAH, ISAS, ISABG, ISAVC>::close_query(
    pud_descent current) {

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

    const uint32_t added_var_count = static_cast<uint32_t>(translation_map.size());
    const pud_node_id node_id = next_node_id_.next();

    store_added_specializations_.store(node_id, std::move(added_specializations));
    store_added_body_goals_.store(node_id, std::move(added_body_goals));
    store_added_var_count_.store(node_id, added_var_count);

    return node_id;
}

#endif
