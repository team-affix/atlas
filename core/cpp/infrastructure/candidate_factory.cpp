#include "../../hpp/infrastructure/candidate_factory.hpp"
#include "../../hpp/domain/value_objects/candidate.hpp"
#include "../../hpp/bootstrap/locator.hpp"

candidate_factory::candidate_factory()
    : unifier_factory_(locator::locate<i_factory<i_unifier, const resolution_lineage*>>()),
      tm_factory_(locator::locate<i_factory<i_translation_map>>()),
      db_(locator::locate<i_database>()),
      copier_(locator::locate<i_copier>()) {}

std::unique_ptr<candidate> candidate_factory::make(const resolution_lineage* rl, const expr* e) {
    auto tm = tm_factory_.make();
    const expr* copied_head = copier_.copy(db_.at(rl->idx).head, *tm);
    auto u = unifier_factory_.make(rl);
    u->push(e, copied_head);
    auto c = std::make_unique<candidate>();
    c->tm = std::move(tm);
    c->u = std::move(u);
    return c;
}
