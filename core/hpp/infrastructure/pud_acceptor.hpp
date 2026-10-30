#ifndef PUD_ACCEPTOR_HPP
#define PUD_ACCEPTOR_HPP

#include <cstdint>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/moduled_expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_substitution.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/moduled_functor.hpp"
#include "value_objects/ubs_class.hpp"
#include "debug_assert.hpp"

template<
    typename IGetNodeGoals,
    typename IGetAxiom,
    typename IGetInference,
    typename IMakeVar,
    typename IMakeFunctor,
    typename IMakeBindMap,
    typename IMakeCycleDetector,
    typename IMakeUnifier,
    typename IMakeExporter>
struct pud_acceptor {
    pud_acceptor(
        IGetNodeGoals& get_node_goals,
        IGetAxiom& get_axiom,
        IGetInference& get_inference,
        IMakeVar& make_var,
        IMakeFunctor& make_functor,
        IMakeBindMap& make_bind_map,
        IMakeCycleDetector& make_cycle_detector,
        IMakeUnifier& make_unifier,
        IMakeExporter& make_exporter);
    pud_inference accept(
        pud_global_node_id acceptor,
        pud_body_goal_id call_site,
        const pud_ubs_state& acceptor_ubs_state,
        uint32_t acceptor_lvc,
        const expr* call_site_goal,
        const std::vector<pud_global_node_id>& candidate_path);
private:
    void note_caller_classes(
        const std::optional<std::vector<ubs_class>>& edited,
        uint32_t acceptor_lvc,
        std::set<uint32_t>& touched_caller_reps);

    IGetNodeGoals& get_node_goals_;
    IGetAxiom& get_axiom_;
    IGetInference& get_inference_;
    IMakeVar& make_var_;
    IMakeFunctor& make_functor_;
    IMakeBindMap& make_bind_map_;
    IMakeCycleDetector& make_cycle_detector_;
    IMakeUnifier& make_unifier_;
    IMakeExporter& make_exporter_;
};

#define PUD_ACCEPTOR_TEMPLATE                                                              \
    template<typename IGetNodeGoals, typename IGetAxiom, typename IGetInference, typename IMakeVar, \
             typename IMakeFunctor, typename IMakeBindMap, typename IMakeCycleDetector, typename IMakeUnifier,    \
             typename IMakeExporter>
#define PUD_ACCEPTOR                                                                       \
    pud_acceptor<IGetNodeGoals, IGetAxiom, IGetInference, IMakeVar, IMakeFunctor, IMakeBindMap,          \
        IMakeCycleDetector, IMakeUnifier, IMakeExporter>

PUD_ACCEPTOR_TEMPLATE
PUD_ACCEPTOR::pud_acceptor(
    IGetNodeGoals& get_node_goals,
    IGetAxiom& get_axiom,
    IGetInference& get_inference,
    IMakeVar& make_var,
    IMakeFunctor& make_functor,
    IMakeBindMap& make_bind_map,
    IMakeCycleDetector& make_cycle_detector,
    IMakeUnifier& make_unifier,
    IMakeExporter& make_exporter)
    : get_node_goals_(get_node_goals)
    , get_axiom_(get_axiom)
    , get_inference_(get_inference)
    , make_var_(make_var)
    , make_functor_(make_functor)
    , make_bind_map_(make_bind_map)
    , make_cycle_detector_(make_cycle_detector)
    , make_unifier_(make_unifier)
    , make_exporter_(make_exporter) {}

PUD_ACCEPTOR_TEMPLATE
pud_inference PUD_ACCEPTOR::accept(
    pud_global_node_id acceptor,
    pud_body_goal_id call_site,
    const pud_ubs_state& acceptor_ubs_state,
    uint32_t acceptor_lvc,
    const expr* call_site_goal,
    const std::vector<pud_global_node_id>& candidate_path) {
    auto&& accepted = make_bind_map_.make_bind_map(acceptor_ubs_state);
    auto&& cycle_detector = make_cycle_detector_.make_cycle_detector(accepted);
    auto&& unifier = make_unifier_.make_unifier(accepted, cycle_detector);
    std::set<uint32_t> touched_caller_reps;
    const rule& axiom = get_axiom_.get_axiom(candidate_path.front());
    note_caller_classes(
        unifier.unify(
            moduled_expr{call_site_goal, 0, acceptor.module},
            moduled_expr{axiom.head, acceptor_lvc, candidate_path.front().module}),
        acceptor_lvc,
        touched_caller_reps);
    for (size_t path_index = 1; path_index < candidate_path.size(); ++path_index) {
        const pud_global_node_id path_node = candidate_path.at(path_index);
        for (const pud_substitution& substitution : get_inference_.get_inference(path_node).substitutions) {
            const moduled_expr replaced{make_var_.make_var(substitution.var_idx), acceptor_lvc, path_node.module};
            const moduled_expr value{substitution.value, acceptor_lvc, path_node.module};
            note_caller_classes(unifier.unify(replaced, value), acceptor_lvc, touched_caller_reps);
        }
    }

    std::unordered_map<uint32_t, uint32_t> translations;
    auto&& exporter = make_exporter_.make_exporter(accepted, acceptor_lvc, translations);
    pud_inference inference{
        .parent = acceptor.node, .call_site = call_site, .substitutions = {}, .body_goals = {}, .var_count = 0};
    for (uint32_t caller_rep : touched_caller_reps) {
        const ubs_class accepted_class = accepted.find(caller_rep);
        const bool unchanged = accepted_class.rep == caller_rep && !accepted_class.value.has_value();
        if (unchanged)
            continue;
        inference.substitutions.push_back(pud_substitution{
            .var_idx = caller_rep,
            .value = accepted_class.rep != caller_rep
                ? make_var_.make_var(accepted_class.rep)
                : exporter.export_expr(
                      moduled_expr{
                          make_functor_.make_functor(
                              accepted_class.value->functor->id, accepted_class.value->functor->args),
                          accepted_class.value->frame_offset,
                          accepted_class.value->module},
                      acceptor.module),
        });
    }
    for (const auto& [id, callee_goal] : get_node_goals_.get(candidate_path.back()).goals)
        inference.body_goals.push_back(exporter.export_expr(
            moduled_expr{callee_goal, acceptor_lvc, candidate_path.back().module}, acceptor.module));
    inference.var_count = static_cast<uint32_t>(translations.size());
    return inference;
}

PUD_ACCEPTOR_TEMPLATE
void PUD_ACCEPTOR::note_caller_classes(
    const std::optional<std::vector<ubs_class>>& edited,
    uint32_t acceptor_lvc,
    std::set<uint32_t>& touched_caller_reps) {
    DEBUG_ASSERT(edited.has_value());
    if (!edited.has_value())
        return;
    for (const ubs_class& edited_class : edited.value()) {
        const bool unbound_caller_class = edited_class.rep < acceptor_lvc && !edited_class.value.has_value();
        if (!unbound_caller_class)
            continue;
        touched_caller_reps.insert(edited_class.rep);
    }
}

#undef PUD_ACCEPTOR
#undef PUD_ACCEPTOR_TEMPLATE

#endif
