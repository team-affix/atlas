#ifndef PUD_WITNESS_SEARCH_HEAD_FORKER_HPP
#define PUD_WITNESS_SEARCH_HEAD_FORKER_HPP

// Forks witness search heads of the given type onto a new search root descent.
template<typename Head, typename Descent>
struct pud_witness_search_head_forker {
    Head fork(const Head& other, Descent search_root_descent) const;
};

template<typename H, typename D>
H pud_witness_search_head_forker<H, D>::fork(const H& other, D search_root_descent) const {
    return H{other, search_root_descent};
}

#endif
