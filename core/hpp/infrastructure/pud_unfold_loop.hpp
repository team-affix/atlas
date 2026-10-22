#ifndef PUD_UNFOLD_LOOP_HPP
#define PUD_UNFOLD_LOOP_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/pud_unfold.hpp"

template<typename IGetOpenLeaves, typename IGetNextUnfold, typename IExpand>
struct pud_unfold_loop {
    pud_unfold_loop(
        IGetOpenLeaves& get_open_leaves,
        IGetNextUnfold& get_next_unfold,
        IExpand& expand);
    coroutine<pud_unfold, void> drive_unfold_loop();
private:
    IGetOpenLeaves& get_open_leaves_;
    IGetNextUnfold& get_next_unfold_;
    IExpand& expand_;
};

template<typename IGOL, typename IGNU, typename IE>
pud_unfold_loop<IGOL, IGNU, IE>::pud_unfold_loop(
    IGOL& get_open_leaves,
    IGNU& get_next_unfold,
    IE& expand)
    : get_open_leaves_(get_open_leaves)
    , get_next_unfold_(get_next_unfold)
    , expand_(expand) {}

template<typename IGOL, typename IGNU, typename IE>
coroutine<pud_unfold, void> pud_unfold_loop<IGOL, IGNU, IE>::drive_unfold_loop() {
    while (!get_open_leaves_.get().empty()) {
        const pud_unfold next = get_next_unfold_.get_next_unfold(get_open_leaves_.get());
        expand_.expand(next.leaf, next.goal);
        co_yield next;
    }
}

#endif
