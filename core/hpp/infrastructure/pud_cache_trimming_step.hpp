#ifndef PUD_CACHE_TRIMMING_STEP_HPP
#define PUD_CACHE_TRIMMING_STEP_HPP

#include "value_objects/pud_unfold_outcome.hpp"

// Makes one search step, then trims a cache back to its budget. Between steps nothing holds a reference into the
// cache, so this is where it may drop entries.
template<typename IUnfoldOne, typename ITrimCache>
struct pud_cache_trimming_step {
    pud_cache_trimming_step(IUnfoldOne& unfold_one, ITrimCache& trim_cache);
    pud_unfold_outcome unfold_one();
private:
    IUnfoldOne& unfold_one_;
    ITrimCache& trim_cache_;
};

template<typename IUnfoldOne, typename ITrimCache>
pud_cache_trimming_step<IUnfoldOne, ITrimCache>::pud_cache_trimming_step(IUnfoldOne& unfold_one, ITrimCache& trim_cache)
    : unfold_one_(unfold_one)
    , trim_cache_(trim_cache) {}

template<typename IUnfoldOne, typename ITrimCache>
pud_unfold_outcome pud_cache_trimming_step<IUnfoldOne, ITrimCache>::unfold_one() {
    const pud_unfold_outcome outcome = unfold_one_.unfold_one();
    trim_cache_.trim();
    return outcome;
}

#endif
