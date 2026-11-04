#ifndef PUD_LEAF_EVALUATION_STORE_HPP
#define PUD_LEAF_EVALUATION_STORE_HPP

#include "value_objects/pud_leaf_evaluation.hpp"

// Holds the evaluation of the leaf being visited, for the parts of the visit that read it after it was made.
struct pud_leaf_evaluation_store {
    pud_leaf_evaluation_store();
    void store(pud_leaf_evaluation evaluation);
    const pud_leaf_evaluation& get() const;
private:
    pud_leaf_evaluation evaluation_;
};

#endif
