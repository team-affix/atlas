#ifndef PUD_EAGER_GOAL_QUERIER_HPP
#define PUD_EAGER_GOAL_QUERIER_HPP

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_specialization_state.hpp"
#include "value_objects/pud_ubs_state.hpp"

// The MHQ query without its stored queries: finds a goal's candidates now. Every axiom the goal's module sees whose
// head resolves the goal is specialized from its root, and kept if it is justified. Nothing outlives the call.
template<typename IEnterReplay, typename IListVisibleAxioms, typename ISpecializeCandidate>
struct pud_eager_goal_querier {
    pud_eager_goal_querier(
        IEnterReplay& enter_replay,
        IListVisibleAxioms& list_visible_axioms,
        ISpecializeCandidate& specialize_candidate);
    std::vector<pud_eager_candidate> query(
        pud_module_id module, const pud_ubs_state& caller_bindings, const expr* goal, uint32_t frame_offset) const;
private:
    IEnterReplay& enter_replay_;
    IListVisibleAxioms& list_visible_axioms_;
    ISpecializeCandidate& specialize_candidate_;
};

#define PUD_EAGER_GOAL_QUERIER_TEMPLATE                                                                 \
    template<typename IEnterReplay, typename IListVisibleAxioms, typename ISpecializeCandidate>
#define PUD_EAGER_GOAL_QUERIER pud_eager_goal_querier<IEnterReplay, IListVisibleAxioms, ISpecializeCandidate>

PUD_EAGER_GOAL_QUERIER_TEMPLATE
PUD_EAGER_GOAL_QUERIER::pud_eager_goal_querier(
    IEnterReplay& enter_replay,
    IListVisibleAxioms& list_visible_axioms,
    ISpecializeCandidate& specialize_candidate)
    : enter_replay_(enter_replay)
    , list_visible_axioms_(list_visible_axioms)
    , specialize_candidate_(specialize_candidate) {}

PUD_EAGER_GOAL_QUERIER_TEMPLATE
std::vector<pud_eager_candidate> PUD_EAGER_GOAL_QUERIER::query(
    pud_module_id module, const pud_ubs_state& caller_bindings, const expr* goal, uint32_t frame_offset) const {
    std::vector<pud_eager_candidate> candidates;
    for (const pud_global_node_id axiom : list_visible_axioms_.list_visible_axioms(module)) {
        std::optional<pud_replay> axiom_root = enter_replay_.enter(caller_bindings, goal, frame_offset, axiom, module);
        if (!axiom_root.has_value())
            continue;
        pud_specialization_state state{
            .path = {axiom_root->node}, .current = std::move(axiom_root.value()), .witnesses = std::nullopt};
        if (!specialize_candidate_.specialize(state))
            continue;
        pud_eager_candidate found{.path = std::move(state.path), .witness_leaves = {}};
        if (!state.witnesses.has_value())
            found.witness_leaves.push_back(state.current.node);
        else
            found.witness_leaves = {state.witnesses->a->leaf, state.witnesses->b->leaf};
        candidates.push_back(std::move(found));
    }
    return candidates;
}

#undef PUD_EAGER_GOAL_QUERIER
#undef PUD_EAGER_GOAL_QUERIER_TEMPLATE

#endif
