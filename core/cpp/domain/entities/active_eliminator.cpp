#include "../../../hpp/domain/entities/active_eliminator.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

active_eliminator::active_eliminator()
    : f(locator::locate<i_frontier>()) {}

void active_eliminator::eliminate(const resolution_lineage* rl) {
    current_rl = rl;
    f.at(rl->parent)->candidates.erase(rl->idx);
}
