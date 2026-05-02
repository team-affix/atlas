#include "../../hpp/infrastructure/decision_store.hpp"

void decision_store::insert(const resolution_lineage* rl) {
    decisions.insert(rl);
}

void decision_store::clear() {
    decisions.clear();
}

size_t decision_store::size() {
    return decisions.size();
}

lemma decision_store::derive_lemma() {
    return lemma{decisions};
}
