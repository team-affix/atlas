#include "../../../hpp/domain/value_objects/candidate.hpp"
#include "../../../hpp/bootstrap/locator.hpp"
#include "../../../hpp/domain/interfaces/i_database.hpp"
#include "../../../hpp/domain/interfaces/i_copier.hpp"

candidate::candidate(const resolution_lineage* lineage) {
    const auto& db = locator::locate<i_database>();
    const auto& c = locator::locate<i_copier>();
    copied_head = c.copy(db.at(lineage->idx).head, tm);
}
