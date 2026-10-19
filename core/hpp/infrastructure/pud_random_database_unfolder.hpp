#ifndef PUD_RANDOM_DATABASE_UNFOLDER_HPP
#define PUD_RANDOM_DATABASE_UNFOLDER_HPP

#include <set>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_derivation_id.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_unfold_choice.hpp"

template<typename IOpenDatabase, typename IUnfold, typename IChooseUnfold>
struct pud_random_database_unfolder {
    pud_random_database_unfolder(
        IOpenDatabase& open_database,
        IUnfold& unfold,
        IChooseUnfold& choose_unfold);
    coroutine<pud_frontier_change, void> unfold_database();
private:
    void track(const pud_frontier_change& change);

    IOpenDatabase& open_database_;
    IUnfold& unfold_;
    IChooseUnfold& choose_unfold_;
    std::set<pud_derivation_id> frontier_;
};

template<typename IOD, typename IU, typename ICU>
pud_random_database_unfolder<IOD, IU, ICU>::pud_random_database_unfolder(
    IOD& open_database,
    IU& unfold,
    ICU& choose_unfold)
    : open_database_(open_database)
    , unfold_(unfold)
    , choose_unfold_(choose_unfold)
    , frontier_() {}

template<typename IOD, typename IU, typename ICU>
coroutine<pud_frontier_change, void> pud_random_database_unfolder<IOD, IU, ICU>::unfold_database() {
    const pud_frontier_change opened = open_database_.open_database();
    track(opened);
    co_yield opened;
    while (!frontier_.empty()) {
        const pud_unfold_choice choice = choose_unfold_.choose_unfold(frontier_);
        frontier_.erase(choice.derivation);
        const pud_frontier_change change = unfold_.unfold(choice.derivation, choice.goal);
        track(change);
        co_yield change;
    }
}

template<typename IOD, typename IU, typename ICU>
void pud_random_database_unfolder<IOD, IU, ICU>::track(const pud_frontier_change& change) {
    for (pud_derivation_id derivation : change.opened)
        frontier_.insert(derivation);
    for (pud_derivation_id derivation : change.refuted)
        frontier_.erase(derivation);
}

#endif
