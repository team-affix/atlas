#ifndef PUD_QUERY_OPENER_HPP
#define PUD_QUERY_OPENER_HPP

#include <cstdint>
#include <vector>
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

template<
    typename ReplayDescent,
    typename BindMap,
    typename CycleDetector,
    typename Unifier,
    typename IGetAxioms,
    typename IGetNodeHead,
    typename IMakeVar,
    typename IMakeCycleDetector,
    typename IMakeUnifier,
    typename IGetNodeAddedSubstitutions>
struct pud_query_opener {
    pud_query_opener(
        IGetAxioms& get_axioms,
        IGetNodeHead& get_node_head,
        IMakeVar& make_var,
        IMakeCycleDetector& make_cycle_detector,
        IMakeUnifier& make_unifier,
        IGetNodeAddedSubstitutions& get_node_added_substitutions);
    std::vector<ReplayDescent> open(
        const pud_ubs_state& caller_ubs_state,
        const expr* goal,
        uint32_t frame_offset);
private:
    IGetAxioms& get_axioms_;
    IGetNodeHead& get_node_head_;
    IMakeVar& make_var_;
    IMakeCycleDetector& make_cycle_detector_;
    IMakeUnifier& make_unifier_;
    IGetNodeAddedSubstitutions& get_node_added_substitutions_;
};

#define PUD_QUERY_OPENER_TEMPLATE                                                          \
    template<typename ReplayDescent, typename BindMap, typename CycleDetector, typename Unifier, \
             typename IGetAxioms, typename IGetNodeHead, typename IMakeVar,                  \
             typename IMakeCycleDetector, typename IMakeUnifier,         \
             typename IGetNodeAddedSubstitutions>
#define PUD_QUERY_OPENER                                                                   \
    pud_query_opener<ReplayDescent, BindMap, CycleDetector, Unifier, IGetAxioms, IGetNodeHead, \
        IMakeVar, IMakeCycleDetector, IMakeUnifier, IGetNodeAddedSubstitutions>

PUD_QUERY_OPENER_TEMPLATE
PUD_QUERY_OPENER::pud_query_opener(
    IGetAxioms& get_axioms,
    IGetNodeHead& get_node_head,
    IMakeVar& make_var,
    IMakeCycleDetector& make_cycle_detector,
    IMakeUnifier& make_unifier,
    IGetNodeAddedSubstitutions& get_node_added_substitutions)
    : get_axioms_(get_axioms)
    , get_node_head_(get_node_head)
    , make_var_(make_var)
    , make_cycle_detector_(make_cycle_detector)
    , make_unifier_(make_unifier)
    , get_node_added_substitutions_(get_node_added_substitutions) {}

PUD_QUERY_OPENER_TEMPLATE
std::vector<ReplayDescent> PUD_QUERY_OPENER::open(
    const pud_ubs_state& caller_ubs_state,
    const expr* goal,
    uint32_t frame_offset) {
    std::vector<ReplayDescent> axiom_descents;
    for (pud_node_id axiom : get_axioms_.get()) {
        pud_transient_ubs_state transient_ubs_state{caller_ubs_state};
        BindMap bind_map = transient_ubs_state.make_bind_map();
        CycleDetector cycle_detector = make_cycle_detector_.make(bind_map);
        Unifier unifier = make_unifier_.make(cycle_detector, bind_map);
        if (!unifier.unify(framed_expr{goal, 0}, framed_expr{get_node_head_.get(axiom), frame_offset}))
            continue;
        axiom_descents.push_back(ReplayDescent{
            make_var_,
            make_cycle_detector_,
            make_unifier_,
            get_node_added_substitutions_,
            axiom,
            transient_ubs_state.to_persistent_state(),
            frame_offset});
    }
    return axiom_descents;
}

#undef PUD_QUERY_OPENER
#undef PUD_QUERY_OPENER_TEMPLATE

#endif
