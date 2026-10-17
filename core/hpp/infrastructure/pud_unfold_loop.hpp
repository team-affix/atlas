#ifndef PUD_UNFOLD_LOOP_HPP
#define PUD_UNFOLD_LOOP_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/pud_unfold.hpp"

// Unfolds once per resume: the injected next-unfold chooses one of the injected open leaves and one of
// its goals, and that leaf is unfolded. The loop ends once no leaf is open.
template<typename IGetOpenLeaves, typename IGetNextUnfold, typename IUnfold>
struct pud_unfold_loop {
    pud_unfold_loop(
        IGetOpenLeaves& get_open_leaves,
        IGetNextUnfold& get_next_unfold,
        IUnfold& unfold);
    coroutine<pud_unfold, void> drive_unfold_loop();
private:
    IGetOpenLeaves& get_open_leaves_;
    IGetNextUnfold& get_next_unfold_;
    IUnfold& unfold_;
};

template<typename IGOL, typename IGNU, typename IU>
pud_unfold_loop<IGOL, IGNU, IU>::pud_unfold_loop(
    IGOL& get_open_leaves,
    IGNU& get_next_unfold,
    IU& unfold)
    : get_open_leaves_(get_open_leaves)
    , get_next_unfold_(get_next_unfold)
    , unfold_(unfold) {}

template<typename IGOL, typename IGNU, typename IU>
coroutine<pud_unfold, void> pud_unfold_loop<IGOL, IGNU, IU>::drive_unfold_loop() {
    while (!get_open_leaves_.get().empty()) {
        const pud_unfold next = get_next_unfold_.get_next_unfold(get_open_leaves_.get());
        unfold_.unfold(next.leaf, next.goal);
        co_yield next;
    }
}

#endif
