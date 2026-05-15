#include "../../hpp/infrastructure/candidate_initializer.hpp"
#include "../../hpp/bootstrap/locator.hpp"

candidate_initializer::candidate_initializer() :
    db_(locator::locate<i_database>()),
    frontier_(locator::locate<i_frontier>()),
    copier_(locator::locate<i_copier>()),
    candidates_(nullptr) {
}

void candidate_initializer::seed_expansion(const goal_lineage* gl) {
    candidates_ = &frontier_.at(gl)->candidates;
}

void candidate_initializer::initialize(const resolution_lineage* rl) {
    auto& c = candidates_->at(rl->idx);
    c->copied_head = copier_.copy(db_.at(rl->idx).head, c->tm);
}
