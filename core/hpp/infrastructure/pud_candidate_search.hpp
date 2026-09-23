#ifndef PUD_CANDIDATE_SEARCH_HPP
#define PUD_CANDIDATE_SEARCH_HPP

template<typename IWitnessSearch>
struct pud_candidate_search {
private:
    IWitnessSearch witness_search_a_;
    IWitnessSearch witness_search_b_;
};

#endif
