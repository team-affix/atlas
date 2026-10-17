#ifndef PUD_QUERY_HEAD_FORKER_HPP
#define PUD_QUERY_HEAD_FORKER_HPP

#include <vector>

// Forks query heads of the given type onto a new set of root descents.
template<typename Head, typename ReplayDescent>
struct pud_query_head_forker {
    Head fork(const Head& other, const std::vector<ReplayDescent>& root_descents) const;
};

template<typename H, typename RD>
H pud_query_head_forker<H, RD>::fork(const H& other, const std::vector<RD>& root_descents) const {
    return H{other, root_descents};
}

#endif
