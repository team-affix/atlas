#ifndef PUD_UNFOLD_LOOP_HPP
#define PUD_UNFOLD_LOOP_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/pud_decision.hpp"

// Unfolds one decision per resume: the injected decision generator chooses one of the injected open
// leaves and one of its goals, and that leaf is unfolded. The loop ends once no leaf is open.
template<typename IGetOpenLeaves, typename IGenerateDecision, typename IUnfold>
struct pud_unfold_loop {
    pud_unfold_loop(
        IGetOpenLeaves& get_open_leaves,
        IGenerateDecision& generate_decision,
        IUnfold& unfold);
    coroutine<pud_decision, void> drive_unfold_loop();
private:
    IGetOpenLeaves& get_open_leaves_;
    IGenerateDecision& generate_decision_;
    IUnfold& unfold_;
};

template<typename IGOL, typename IGD, typename IU>
pud_unfold_loop<IGOL, IGD, IU>::pud_unfold_loop(
    IGOL& get_open_leaves,
    IGD& generate_decision,
    IU& unfold)
    : get_open_leaves_(get_open_leaves)
    , generate_decision_(generate_decision)
    , unfold_(unfold) {}

template<typename IGOL, typename IGD, typename IU>
coroutine<pud_decision, void> pud_unfold_loop<IGOL, IGD, IU>::drive_unfold_loop() {
    while (!get_open_leaves_.get().empty()) {
        const pud_decision decision = generate_decision_.generate(get_open_leaves_.get());
        unfold_.unfold(decision.leaf, decision.goal);
        co_yield decision;
    }
}

#endif
