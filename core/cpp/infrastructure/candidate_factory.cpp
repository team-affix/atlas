#include "../../hpp/infrastructure/candidate_factory.hpp"
#include "../../hpp/domain/entities/candidate.hpp"
#include "../../hpp/bootstrap/locator.hpp"

candidate_factory::candidate_factory()
    : unifier_factory_(locator::locate<i_factory<i_unifier, const resolution_lineage*>>()),
      tm_factory_(locator::locate<i_factory<i_translation_map>>()),
      db_(locator::locate<i_database>()),
      copier_(locator::locate<i_copier>()) {}

std::unique_ptr<i_candidate> candidate_factory::make(const resolution_lineage* rl, const expr* e) {
    auto tm = tm_factory_.make();
    const expr* copied_head = copier_.copy(db_.at(rl->idx).head, *tm);
    auto u = unifier_factory_.make(rl);
    u->push(e, copied_head);
    return std::make_unique<candidate>(std::move(tm), std::move(u));
}
