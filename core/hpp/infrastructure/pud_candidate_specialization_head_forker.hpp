#ifndef PUD_CANDIDATE_SPECIALIZATION_HEAD_FORKER_HPP
#define PUD_CANDIDATE_SPECIALIZATION_HEAD_FORKER_HPP

// Forks candidate specialization heads of the given type onto a new specialization root descent.
template<typename Head, typename Descent>
struct pud_candidate_specialization_head_forker {
    Head fork(const Head& other, Descent search_root_descent) const;
};

template<typename H, typename D>
H pud_candidate_specialization_head_forker<H, D>::fork(const H& other, D search_root_descent) const {
    return H{other, search_root_descent};
}

#endif
