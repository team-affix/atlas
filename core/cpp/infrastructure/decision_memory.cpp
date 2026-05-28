#include "../../hpp/infrastructure/decision_memory.hpp"

void decision_memory::record_decision(const resolution_lineage* rl) {
    decisions.insert(rl);
}

void decision_memory::clear_decision_record() {
    decisions.clear();
}

size_t decision_memory::count() const {
    return decisions.size();
}

lemma decision_memory::derive() const {
    return lemma{decisions};
}
