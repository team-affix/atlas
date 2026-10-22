#ifndef PUD_RANDOM_DATABASE_EXPANDER_HPP
#define PUD_RANDOM_DATABASE_EXPANDER_HPP

#include <set>
#include "infrastructure/coroutine.hpp"
#include "value_objects/pud_derivation_id.hpp"
#include "value_objects/pud_expansion_choice.hpp"
#include "value_objects/pud_frontier_change.hpp"

template<typename IOpenDatabase, typename IExpand, typename IChooseExpansion>
struct pud_random_database_expander {
    pud_random_database_expander(
        IOpenDatabase& open_database,
        IExpand& expand,
        IChooseExpansion& choose_expansion);
    coroutine<pud_frontier_change, void> expand_database();
private:
    void track(const pud_frontier_change& change);

    IOpenDatabase& open_database_;
    IExpand& expand_;
    IChooseExpansion& choose_expansion_;
    std::set<pud_derivation_id> frontier_;
};

template<typename IOD, typename IE, typename ICE>
pud_random_database_expander<IOD, IE, ICE>::pud_random_database_expander(
    IOD& open_database,
    IE& expand,
    ICE& choose_expansion)
    : open_database_(open_database)
    , expand_(expand)
    , choose_expansion_(choose_expansion)
    , frontier_() {}

template<typename IOD, typename IE, typename ICE>
coroutine<pud_frontier_change, void> pud_random_database_expander<IOD, IE, ICE>::expand_database() {
    const pud_frontier_change opened = open_database_.open_database();
    track(opened);
    co_yield opened;
    while (!frontier_.empty()) {
        const pud_expansion_choice choice = choose_expansion_.choose_expansion(frontier_);
        frontier_.erase(choice.derivation);
        const pud_frontier_change change = expand_.expand(choice.derivation, choice.goal);
        track(change);
        co_yield change;
    }
}

template<typename IOD, typename IE, typename ICE>
void pud_random_database_expander<IOD, IE, ICE>::track(const pud_frontier_change& change) {
    for (pud_derivation_id derivation : change.opened)
        frontier_.insert(derivation);
    for (pud_derivation_id derivation : change.refuted)
        frontier_.erase(derivation);
}

#endif
