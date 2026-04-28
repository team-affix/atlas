#include "../../../../hpp/domain/entities/decision_store/decision_store.hpp"

decision_store::decision_store() :
    decisions() {
}

void decision_store::insert(const resolution_lineage* rl) {
    decisions.insert(rl);
}

const std::unordered_set<const resolution_lineage*>& decision_store::get() const {
    return decisions;
}

void decision_store::clear() {
    decisions.clear();
}
