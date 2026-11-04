#include "infrastructure/pud_leaf_evaluation_store.hpp"
#include <utility>

pud_leaf_evaluation_store::pud_leaf_evaluation_store()
    : evaluation_(pud_leaf_evaluation{.leaf = {}, .refuted = false, .goals = {}}) {}

void pud_leaf_evaluation_store::store(pud_leaf_evaluation evaluation) {
    evaluation_ = std::move(evaluation);
}

const pud_leaf_evaluation& pud_leaf_evaluation_store::get() const {
    return evaluation_;
}
