#include "../../../../hpp/domain/entities/decision_store/decision_store.hpp"
#include "../../../../hpp/infrastructure/locator.hpp"

decision_store::decision_store() :
    decisions(locator::locate<trail>()) {
}

void decision_store::insert(const resolution_lineage* rl) {
    decisions.insert(rl);
}

const std::unordered_set<const resolution_lineage*>& decision_store::get() const {
    return decisions.get();
}
