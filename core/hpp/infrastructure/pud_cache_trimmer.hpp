#ifndef PUD_CACHE_TRIMMER_HPP
#define PUD_CACHE_TRIMMER_HPP

// Trims two caches back to their budgets together.
template<typename ITrimCache0, typename ITrimCache1>
struct pud_cache_trimmer {
    pud_cache_trimmer(ITrimCache0& cache0, ITrimCache1& cache1);
    void trim();
private:
    ITrimCache0& cache0_;
    ITrimCache1& cache1_;
};

template<typename ITrimCache0, typename ITrimCache1>
pud_cache_trimmer<ITrimCache0, ITrimCache1>::pud_cache_trimmer(ITrimCache0& cache0, ITrimCache1& cache1)
    : cache0_(cache0)
    , cache1_(cache1) {}

template<typename ITrimCache0, typename ITrimCache1>
void pud_cache_trimmer<ITrimCache0, ITrimCache1>::trim() {
    cache0_.trim();
    cache1_.trim();
}

#endif
