#include "../../../hpp/domain/entities/candidate_applicant_activator.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

candidate_applicant_activator::candidate_applicant_activator() :
    af(locator::locate<i_applicant_frontier>()),
    u_factory(locator::locate<i_factory<i_unifier, const resolution_lineage*>>()),
    tm_factory(locator::locate<i_factory<i_translation_map>>()),
    db(locator::locate<i_database>()),
    cp(locator::locate<i_copier>()) {}

void candidate_applicant_activator::activate(const resolution_lineage* rl) {
    auto tm = tm_factory.make();
    const expr* copied_head = cp.copy(db.at(rl->idx).head, *tm);
    auto u = u_factory.make(rl);
    af.insert(rl, applicant{rl->idx, copied_head, std::move(tm), std::move(u)});
}
