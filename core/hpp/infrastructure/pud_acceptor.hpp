#ifndef PUD_ACCEPTOR_HPP
#define PUD_ACCEPTOR_HPP

#include <cstdint>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_body_goal.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_node_delta.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_substitution.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/ubs_class.hpp"
#include "debug_assert.hpp"

// Accepts a candidate into an inference of the caller on its call site. The call-site goal enters the
// candidate's axiom and the candidate's path is replayed below it, the callee framed at the caller's
// lvc, in one environment on top of the caller's. Every caller class that was unbound when a
// unification edited it is noted as it is edited; one that ends up bound, or merged under another
// representative, becomes a substitution. The callee's open goals are exported as the inference's
// body goals.
template<
    typename IGetNodeGoals,
    typename IGetAxiom,
    typename IGetInference,
    typename IMakeVar,
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
        IMakeBindMap& make_bind_map,
        IMakeCycleDetector& make_cycle_detector,
        IMakeUnifier& make_unifier,
        IMakeExporter& make_exporter);
    pud_node_delta accept(
        pud_node_id acceptor,
        pud_body_goal_id call_site,
        const pud_ubs_state& acceptor_ubs_state,
        uint32_t acceptor_lvc,
        const expr* call_site_goal,
        const std::vector<pud_node_id>& candidate_path);
private:
    void note_caller_classes(
        const std::optional<std::vector<ubs_class>>& edited,
        uint32_t acceptor_lvc,
        std::set<uint32_t>& touched_caller_reps);

    IGetNodeGoals& get_node_goals_;
    IGetAxiom& get_axiom_;
    IGetInference& get_inference_;
    IMakeVar& make_var_;
    IMakeBindMap& make_bind_map_;
    IMakeCycleDetector& make_cycle_detector_;
    IMakeUnifier& make_unifier_;
    IMakeExporter& make_exporter_;
};

#define PUD_ACCEPTOR_TEMPLATE                                                              \
    template<typename IGetNodeGoals, typename IGetAxiom, typename IGetInference, typename IMakeVar, \
             typename IMakeBindMap, typename IMakeCycleDetector, typename IMakeUnifier,    \
             typename IMakeExporter>
#define PUD_ACCEPTOR                                                                       \
    pud_acceptor<IGetNodeGoals, IGetAxiom, IGetInference, IMakeVar, IMakeBindMap,          \
        IMakeCycleDetector, IMakeUnifier, IMakeExporter>

PUD_ACCEPTOR_TEMPLATE
PUD_ACCEPTOR::pud_acceptor(
    IGetNodeGoals& get_node_goals,
    IGetAxiom& get_axiom,
    IGetInference& get_inference,
    IMakeVar& make_var,
    IMakeBindMap& make_bind_map,
    IMakeCycleDetector& make_cycle_detector,
    IMakeUnifier& make_unifier,
    IMakeExporter& make_exporter)
    : get_node_goals_(get_node_goals)
    , get_axiom_(get_axiom)
    , get_inference_(get_inference)
    , make_var_(make_var)
    , make_bind_map_(make_bind_map)
    , make_cycle_detector_(make_cycle_detector)
    , make_unifier_(make_unifier)
    , make_exporter_(make_exporter) {}

PUD_ACCEPTOR_TEMPLATE
pud_node_delta PUD_ACCEPTOR::accept(
    pud_node_id acceptor,
    pud_body_goal_id call_site,
    const pud_ubs_state& acceptor_ubs_state,
    uint32_t acceptor_lvc,
    const expr* call_site_goal,
    const std::vector<pud_node_id>& candidate_path) {
    auto&& accepted = make_bind_map_.make_bind_map(acceptor_ubs_state);
    auto&& cycle_detector = make_cycle_detector_.make_cycle_detector(accepted);
    auto&& unifier = make_unifier_.make_unifier(accepted, cycle_detector);
    std::set<uint32_t> touched_caller_reps;
    const framed_expr axiom_head{get_axiom_.get_axiom(candidate_path.front()).head, acceptor_lvc};
    note_caller_classes(unifier.unify(framed_expr{call_site_goal, 0}, axiom_head), acceptor_lvc, touched_caller_reps);
    for (size_t path_index = 1; path_index < candidate_path.size(); ++path_index) {
        for (const pud_substitution& substitution : get_inference_.get_inference(candidate_path.at(path_index)).substitutions) {
            const framed_expr replaced{make_var_.make_var(substitution.var_idx), acceptor_lvc};
            const framed_expr value{substitution.value, acceptor_lvc};
            note_caller_classes(unifier.unify(replaced, value), acceptor_lvc, touched_caller_reps);
        }
    }

    std::unordered_map<uint32_t, uint32_t> translations;
    auto&& exporter = make_exporter_.make_exporter(accepted, acceptor_lvc, translations);
    pud_node_delta delta{
        .parent = acceptor, .call_site = call_site, .substitutions = {}, .body_goals = {}, .var_count = 0};
    for (uint32_t caller_rep : touched_caller_reps) {
        const ubs_class accepted_class = accepted.find(caller_rep);
        const bool unchanged = accepted_class.rep == caller_rep && !accepted_class.value.has_value();
        if (unchanged)
            continue;
        delta.substitutions.push_back(pud_substitution{
            .var_idx = caller_rep,
            .value = accepted_class.rep != caller_rep
                ? make_var_.make_var(accepted_class.rep)
                : exporter.export_expr(accepted_class.value.value()),
        });
    }
    for (const pud_body_goal& callee_goal : get_node_goals_.get(candidate_path.back()).goals)
        delta.body_goals.push_back(exporter.export_expr(framed_expr{callee_goal.goal, acceptor_lvc}));
    delta.var_count = static_cast<uint32_t>(translations.size());
    return delta;
}

// The candidate is the query's, so every unification along its path succeeds.
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
