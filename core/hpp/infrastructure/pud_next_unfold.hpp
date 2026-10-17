#ifndef PUD_NEXT_UNFOLD_HPP
#define PUD_NEXT_UNFOLD_HPP

#include <optional>
#include <set>
#include "value_objects/pud_decision.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_unfold.hpp"

// Chooses what to unfold next: forced unit unfolds dominate, wherever they are, and a decision among
// the open leaves is generated only when nothing is forced.
template<typename IGetNextUnitUnfold, typename IGenerateDecision>
struct pud_next_unfold {
    pud_next_unfold(IGetNextUnitUnfold& get_next_unit_unfold, IGenerateDecision& generate_decision);
    pud_unfold get_next_unfold(const std::set<pud_node_id>& open_leaves);
private:
    IGetNextUnitUnfold& get_next_unit_unfold_;
    IGenerateDecision& generate_decision_;
};

template<typename IGNUU, typename IGD>
pud_next_unfold<IGNUU, IGD>::pud_next_unfold(IGNUU& get_next_unit_unfold, IGD& generate_decision)
    : get_next_unit_unfold_(get_next_unit_unfold)
    , generate_decision_(generate_decision) {}

template<typename IGNUU, typename IGD>
pud_unfold pud_next_unfold<IGNUU, IGD>::get_next_unfold(const std::set<pud_node_id>& open_leaves) {
    const std::optional<pud_unfold> unit_unfold = get_next_unit_unfold_.get_next_unit_unfold();
    if (unit_unfold.has_value())
        return unit_unfold.value();
    const pud_decision decision = generate_decision_.generate(open_leaves);
    return pud_unfold{.leaf = decision.leaf, .goal = decision.goal};
}

#endif
