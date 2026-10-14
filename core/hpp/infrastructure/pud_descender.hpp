#ifndef PUD_DESCENDER_HPP
#define PUD_DESCENDER_HPP

#include <optional>
#include <unordered_map>
#include <vector>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include <immer/set.hpp>
#include <immer/set_transient.hpp>
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_descent.hpp"
#include "value_objects/pud_specialization.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

template<
    typename BindMap,
    typename ChangedRepLogger,
    typename Normalizer,
    typename Unifier,
    typename Whnf,
    typename ExportNormalizer,
    typename IMakeChangedRepLogger,
    typename IMakeNormalizer,
    typename IMakeUnifier,
    typename IMakeWhnf,
    typename IMakeExportNormalizer,
    typename IMakeUbsVar,
    typename IMakeUbsFunctor,
    typename INextNodeId,
    typename IMakeVar,
    typename IMakeFunctor,
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
        IMakeChangedRepLogger& make_changed_rep_logger,
        IMakeNormalizer& make_normalizer,
        IMakeUnifier& make_unifier,
        IMakeWhnf& make_whnf,
        IMakeExportNormalizer& make_export_normalizer,
        IMakeUbsVar& make_ubs_var,
        IMakeUbsFunctor& make_ubs_functor,
        IMakeVar& make_var,
        IMakeFunctor& make_functor,
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
    void record_caller_touched_reps(
        immer::set<uint32_t>::transient_type& touched_transient,
        const std::vector<uint32_t>& changed_reps,
        uint32_t caller_global_cutoff);

    INextNodeId& next_node_id_;
    IMakeChangedRepLogger& make_changed_rep_logger_;
    IMakeNormalizer& make_normalizer_;
    IMakeUnifier& make_unifier_;
    IMakeWhnf& make_whnf_;
    IMakeExportNormalizer& make_export_normalizer_;
    IMakeUbsVar& make_ubs_var_;
    IMakeUbsFunctor& make_ubs_functor_;
    IMakeVar& make_var_;
    IMakeFunctor& make_functor_;
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

#define PUD_DESCENDER_TEMPLATE                                                     \
    template<typename BindMap, typename ChangedRepLogger, typename Normalizer, typename Unifier, \
             typename Whnf, typename ExportNormalizer, typename IMakeChangedRepLogger, \
             typename IMakeNormalizer, typename IMakeUnifier, typename IMakeWhnf, \
             typename IMakeExportNormalizer, typename IMakeUbsVar, typename IMakeUbsFunctor, typename INextNodeId, \
             typename IMakeVar, typename IMakeFunctor, typename IGlobalize, typename ICheckNodeRefuted, \
             typename IGetCallSite, typename IGetAddedSpecializations, typename IGetAddedBodyGoals, \
             typename IGetAddedVarCount, typename IGetAxiomHead, typename IStoreAddedSpecializations, \
             typename IStoreAddedBodyGoals, typename IStoreAddedVarCount>
#define PUD_DESCENDER pud_descender<BindMap, ChangedRepLogger, Normalizer, Unifier, Whnf, ExportNormalizer, \
    IMakeChangedRepLogger, IMakeNormalizer, IMakeUnifier, IMakeWhnf, IMakeExportNormalizer, IMakeUbsVar, \
    IMakeUbsFunctor, INextNodeId, IMakeVar, IMakeFunctor, IGlobalize, ICheckNodeRefuted, IGetCallSite, \
    IGetAddedSpecializations, IGetAddedBodyGoals, IGetAddedVarCount, IGetAxiomHead, IStoreAddedSpecializations, \
    IStoreAddedBodyGoals, IStoreAddedVarCount>

PUD_DESCENDER_TEMPLATE
PUD_DESCENDER::pud_descender(
    INextNodeId& next_node_id,
    IMakeChangedRepLogger& make_changed_rep_logger,
    IMakeNormalizer& make_normalizer,
    IMakeUnifier& make_unifier,
    IMakeWhnf& make_whnf,
    IMakeExportNormalizer& make_export_normalizer,
    IMakeUbsVar& make_ubs_var,
    IMakeUbsFunctor& make_ubs_functor,
    IMakeVar& make_var,
    IMakeFunctor& make_functor,
    IGlobalize& globalize,
    ICheckNodeRefuted& check_node_refuted,
    IGetCallSite& get_call_site,
    IGetAddedSpecializations& get_added_specializations,
    IGetAddedBodyGoals& get_added_body_goals,
    IGetAddedVarCount& get_added_var_count,
    IGetAxiomHead& get_axiom_head,
    IStoreAddedSpecializations& store_added_specializations,
    IStoreAddedBodyGoals& store_added_body_goals,
    IStoreAddedVarCount& store_added_var_count)
    : next_node_id_(next_node_id)
    , make_changed_rep_logger_(make_changed_rep_logger)
    , make_normalizer_(make_normalizer)
    , make_unifier_(make_unifier)
    , make_whnf_(make_whnf)
    , make_export_normalizer_(make_export_normalizer)
    , make_ubs_var_(make_ubs_var)
    , make_ubs_functor_(make_ubs_functor)
    , make_var_(make_var)
    , make_functor_(make_functor)
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

PUD_DESCENDER_TEMPLATE
void PUD_DESCENDER::record_caller_touched_reps(
    immer::set<uint32_t>::transient_type& touched_transient,
    const std::vector<uint32_t>& changed_reps,
    uint32_t caller_global_cutoff) {
    for (uint32_t rep : changed_reps) {
        if (rep >= caller_global_cutoff)
            continue;
        touched_transient.insert(rep);
    }
}

PUD_DESCENDER_TEMPLATE
pud_descent PUD_DESCENDER::descent_root(pud_node_id root_id) {
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
        .ubs_state           = {},
        .pending_body_goals  = std::move(body_goals_transient).persistent()
    };
}

PUD_DESCENDER_TEMPLATE
std::optional<pud_descent> PUD_DESCENDER::descend(pud_descent current, pud_node_id child_node_id) {
    if (check_node_refuted_.check_refuted(child_node_id))
        return std::nullopt;

    pud_transient_ubs_state transient_ubs_state{current.ubs_state};
    BindMap bind_map = transient_ubs_state.make_bind_map(make_ubs_var_, make_ubs_functor_);
    std::vector<uint32_t> changed_reps;
    ChangedRepLogger changed_rep_logger =
        make_changed_rep_logger_.make_changed_rep_logger(bind_map, changed_reps);
    Normalizer normalizer = make_normalizer_.make_normalizer(bind_map);
    Unifier framed_unifier =
        make_unifier_.make_unifier(bind_map, normalizer, changed_rep_logger);

    auto touched_transient = current.touched_caller_reps.transient();
    for (pud_specialization spec : get_added_specializations_.get(child_node_id)) {
        changed_reps.clear();
        framed_expr lhs{make_var_.make_var(spec.var_idx), current.frame_offset};
        framed_expr rhs{spec.value, current.frame_offset};
        if (!framed_unifier.unify(lhs, rhs))
            return std::nullopt;
        record_caller_touched_reps(touched_transient, changed_reps, current.frame_offset);
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
        .ubs_state           = transient_ubs_state.to_persistent_state(),
        .pending_body_goals  = std::move(body_goals_transient).persistent()
    };
}

PUD_DESCENDER_TEMPLATE
std::optional<pud_descent> PUD_DESCENDER::open_query(
    pud_descent caller, const expr* query_expr, pud_node_id root_id) {

    const uint32_t query_frame_offset = caller.frame_offset + caller.lvc;
    const expr* axiom_head = get_axiom_head_.get(root_id);
    const uint32_t root_var_count = get_added_var_count_.get(root_id);

    pud_transient_ubs_state transient_ubs_state{caller.ubs_state};
    BindMap bind_map = transient_ubs_state.make_bind_map(make_ubs_var_, make_ubs_functor_);
    std::vector<uint32_t> changed_reps;
    ChangedRepLogger changed_rep_logger =
        make_changed_rep_logger_.make_changed_rep_logger(bind_map, changed_reps);
    Normalizer normalizer = make_normalizer_.make_normalizer(bind_map);
    Unifier framed_unifier =
        make_unifier_.make_unifier(bind_map, normalizer, changed_rep_logger);

    changed_reps.clear();
    const bool unified = framed_unifier.unify(
        framed_expr{query_expr, caller.frame_offset},
        framed_expr{axiom_head, query_frame_offset});
    if (!unified)
        return std::nullopt;

    auto touched_transient = immer::set<uint32_t>{}.transient();
    record_caller_touched_reps(touched_transient, changed_reps, query_frame_offset);

    auto body_goals_transient = immer::map<body_goal_id, const expr*>{}.transient();
    size_t bgc = 0;
    for (const expr* goal : get_added_body_goals_.get(root_id))
        body_goals_transient.set(bgc++, goal);

    return pud_descent{
        .frame_offset        = query_frame_offset,
        .lvc                 = root_var_count,
        .bgc                 = bgc,
        .node                = root_id,
        .touched_caller_reps = std::move(touched_transient).persistent(),
        .ubs_state           = transient_ubs_state.to_persistent_state(),
        .pending_body_goals  = std::move(body_goals_transient).persistent()
    };
}

PUD_DESCENDER_TEMPLATE
pud_node_id PUD_DESCENDER::close_query(pud_descent current) {
    pud_transient_ubs_state transient_ubs_state{current.ubs_state};
    BindMap bind_map = transient_ubs_state.make_bind_map(make_ubs_var_, make_ubs_functor_);
    Whnf whnf = make_whnf_.make_whnf(bind_map);
    ExportNormalizer export_normalizer =
        make_export_normalizer_.make_export_normalizer(bind_map, whnf);

    std::unordered_map<uint32_t, uint32_t> translation_map;

    std::vector<pud_specialization> added_specializations;
    std::vector<const expr*> added_body_goals;

    for (uint32_t rep : current.touched_caller_reps) {
        const expr* value = bind_map.resolve(rep);
        added_specializations.push_back(pud_specialization{
            .var_idx = rep,
            .value   = export_normalizer.normalize({value, 0u}, current.frame_offset, translation_map)
        });
    }

    for (auto const& [id, goal] : current.pending_body_goals) {
        added_body_goals.push_back(
            export_normalizer.normalize({goal, current.frame_offset}, current.frame_offset, translation_map));
    }

    const uint32_t added_var_count = static_cast<uint32_t>(translation_map.size());
    const pud_node_id node_id = next_node_id_.next();

    store_added_specializations_.store(node_id, std::move(added_specializations));
    store_added_body_goals_.store(node_id, std::move(added_body_goals));
    store_added_var_count_.store(node_id, added_var_count);

    return node_id;
}

#undef PUD_DESCENDER
#undef PUD_DESCENDER_TEMPLATE

#endif
