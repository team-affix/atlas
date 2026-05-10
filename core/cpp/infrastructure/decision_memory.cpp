#include "../../hpp/infrastructure/decision_memory.hpp"

void decision_memory::insert(const resolution_lineage* rl) {
    decisions.insert(rl);
}

void decision_memory::clear() {
    decisions.clear();
}

size_t decision_memory::size() const {
    return decisions.size();
}

lemma decision_memory::derive_lemma() const {
    return lemma{decisions};
}
