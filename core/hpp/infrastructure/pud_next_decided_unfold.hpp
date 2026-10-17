#ifndef PUD_NEXT_DECIDED_UNFOLD_HPP
#define PUD_NEXT_DECIDED_UNFOLD_HPP

#include <set>
#include "value_objects/pud_decision.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_unfold.hpp"

// Chooses what to unfold next by decision alone: nothing is ever forced.
template<typename IGenerateDecision>
struct pud_next_decided_unfold {
    pud_next_decided_unfold(IGenerateDecision& generate_decision);
    pud_unfold get_next_unfold(const std::set<pud_node_id>& open_leaves);
private:
    IGenerateDecision& generate_decision_;
};

template<typename IGD>
pud_next_decided_unfold<IGD>::pud_next_decided_unfold(IGD& generate_decision)
    : generate_decision_(generate_decision) {}

template<typename IGD>
pud_unfold pud_next_decided_unfold<IGD>::get_next_unfold(const std::set<pud_node_id>& open_leaves) {
    const pud_decision decision = generate_decision_.generate(open_leaves);
    return pud_unfold{.leaf = decision.leaf, .goal = decision.goal};
}

#endif
