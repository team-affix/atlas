#ifndef PUD_WITNESS_ADVANCE_RESULT_HPP
#define PUD_WITNESS_ADVANCE_RESULT_HPP

template<typename Descent, typename NodeIterator>
struct pud_witness_advance_result {
    Descent     root_descent;
    NodeIterator    root_next_sibling_it;
    NodeIterator    root_end_sibling_it;
};

#endif
