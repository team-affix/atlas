#ifndef PUD_ACCEPTOR_HPP
#define PUD_ACCEPTOR_HPP

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_child_delta.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_substitution.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/ubs_class.hpp"

template<
    typename IGetNodeGoals,
    typename IEnterReplay,
    typename IDescendReplay,
    typename IMakeVar,
    typename IMakeBindMap,
    typename IMakeExporter,
    typename IFindChangedClasses>
struct pud_acceptor {
    pud_acceptor(
        IGetNodeGoals& get_node_goals,
        IEnterReplay& enter_replay,
        IDescendReplay& descend_replay,
        IMakeVar& make_var,
        IMakeBindMap& make_bind_map,
        IMakeExporter& make_exporter,
        IFindChangedClasses& find_changed_classes);
    pud_child_delta accept(
        const pud_ubs_state& acceptor_ubs_state,
        uint32_t acceptor_lvc,
        const expr* call_site_goal,
        const std::vector<pud_node_id>& candidate_path);
private:
    IGetNodeGoals& get_node_goals_;
    IEnterReplay& enter_replay_;
    IDescendReplay& descend_replay_;
    IMakeVar& make_var_;
    IMakeBindMap& make_bind_map_;
    IMakeExporter& make_exporter_;
    IFindChangedClasses& find_changed_classes_;
};

#define PUD_ACCEPTOR_TEMPLATE                                                              \
    template<typename IGetNodeGoals, typename IEnterReplay, typename IDescendReplay,  \
             typename IMakeVar, typename IMakeBindMap, typename IMakeExporter,             \
             typename IFindChangedClasses>
#define PUD_ACCEPTOR                                                                       \
    pud_acceptor<IGetNodeGoals, IEnterReplay, IDescendReplay, IMakeVar, IMakeBindMap,  \
        IMakeExporter, IFindChangedClasses>

PUD_ACCEPTOR_TEMPLATE
PUD_ACCEPTOR::pud_acceptor(
    IGetNodeGoals& get_node_goals,
    IEnterReplay& enter_replay,
    IDescendReplay& descend_replay,
    IMakeVar& make_var,
    IMakeBindMap& make_bind_map,
    IMakeExporter& make_exporter,
    IFindChangedClasses& find_changed_classes)
    : get_node_goals_(get_node_goals)
    , enter_replay_(enter_replay)
    , descend_replay_(descend_replay)
    , make_var_(make_var)
    , make_bind_map_(make_bind_map)
    , make_exporter_(make_exporter)
    , find_changed_classes_(find_changed_classes) {}

// The candidate is the query's, so its path replays. A caller class changes when it was unbound and
// the resolution binds it or merges it under a smaller representative; each such representative gets
// one substitution.
PUD_ACCEPTOR_TEMPLATE
pud_child_delta PUD_ACCEPTOR::accept(
    const pud_ubs_state& acceptor_ubs_state,
    uint32_t acceptor_lvc,
    const expr* call_site_goal,
    const std::vector<pud_node_id>& candidate_path) {
    pud_replay accepted_replay =
        enter_replay_.enter(acceptor_ubs_state, call_site_goal, acceptor_lvc, candidate_path.front()).value();
    for (size_t path_index = 1; path_index < candidate_path.size(); ++path_index)
        accepted_replay = descend_replay_.descend(accepted_replay, candidate_path.at(path_index)).value();
    const pud_ubs_state& accepted_ubs_state = accepted_replay.ubs_state;

    auto&& accepted = make_bind_map_.make_bind_map(accepted_ubs_state);
    std::vector<std::pair<uint32_t, ubs_class>> changed;
    for (const ubs_class& caller_class : find_changed_classes_.find_changed_classes(acceptor_ubs_state, accepted_ubs_state)) {
        if (caller_class.rep >= acceptor_lvc || caller_class.value.has_value())
            continue;
        const ubs_class accepted_class = accepted.find(caller_class.rep);
        const bool unchanged = accepted_class.rep == caller_class.rep && !accepted_class.value.has_value();
        if (unchanged)
            continue;
        changed.emplace_back(caller_class.rep, accepted_class);
    }
    std::sort(changed.begin(), changed.end());

    auto&& exporter = make_exporter_.make_exporter(accepted, acceptor_lvc);
    pud_child_delta delta{.substitutions = {}, .body_goals = {}, .var_count = 0};
    for (const auto& [caller_rep, accepted_class] : changed) {
        delta.substitutions.push_back(pud_substitution{
            .var_idx = caller_rep,
            .value = accepted_class.rep != caller_rep
                ? make_var_.make_var(accepted_class.rep)
                : exporter.export_expr(accepted_class.value.value()),
        });
    }
    for (const pud_body_goal& callee_goal : get_node_goals_.get(candidate_path.back()).goals)
        delta.body_goals.push_back(exporter.export_expr(framed_expr{callee_goal.goal, acceptor_lvc}));
    delta.var_count = exporter.fresh_var_count();
    return delta;
}

#undef PUD_ACCEPTOR
#undef PUD_ACCEPTOR_TEMPLATE

#endif
