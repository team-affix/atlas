#ifndef PUD_UNIT_UNFOLD_FINDER_HPP
#define PUD_UNIT_UNFOLD_FINDER_HPP

#include <optional>
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_unfold.hpp"

// Finds an unfold forced by a unit query: one of its holders, on its goal. Unit queries are served
// oldest first, each until no leaf holds it any more.
template<typename ICheckUnitQueriesEmpty, typename IFrontUnitQuery, typename IPopUnitQuery, typename IFindQueryHolder>
struct pud_unit_unfold_finder {
    pud_unit_unfold_finder(
        ICheckUnitQueriesEmpty& check_unit_queries_empty,
        IFrontUnitQuery& front_unit_query,
        IPopUnitQuery& pop_unit_query,
        IFindQueryHolder& find_query_holder);
    std::optional<pud_unfold> find_unit_unfold();
private:
    ICheckUnitQueriesEmpty& check_unit_queries_empty_;
    IFrontUnitQuery& front_unit_query_;
    IPopUnitQuery& pop_unit_query_;
    IFindQueryHolder& find_query_holder_;
};

template<typename ICUQE, typename IFUQ, typename IPUQ, typename IFQH>
pud_unit_unfold_finder<ICUQE, IFUQ, IPUQ, IFQH>::pud_unit_unfold_finder(
    ICUQE& check_unit_queries_empty,
    IFUQ& front_unit_query,
    IPUQ& pop_unit_query,
    IFQH& find_query_holder)
    : check_unit_queries_empty_(check_unit_queries_empty)
    , front_unit_query_(front_unit_query)
    , pop_unit_query_(pop_unit_query)
    , find_query_holder_(find_query_holder) {}

template<typename ICUQE, typename IFUQ, typename IPUQ, typename IFQH>
std::optional<pud_unfold> pud_unit_unfold_finder<ICUQE, IFUQ, IPUQ, IFQH>::find_unit_unfold() {
    while (!check_unit_queries_empty_.empty()) {
        const std::optional<pud_unfold> held = find_query_holder_.find_holder(front_unit_query_.front());
        if (held.has_value())
            return held;
        pop_unit_query_.pop_front();
    }
    return std::nullopt;
}

#endif
