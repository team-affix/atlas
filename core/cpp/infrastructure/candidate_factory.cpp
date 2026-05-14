#include "../../hpp/infrastructure/candidate_factory.hpp"
#include "../../hpp/domain/value_objects/candidate.hpp"
#include "../../hpp/bootstrap/locator.hpp"

candidate_factory::candidate_factory() :
    db_(locator::locate<i_database>()),
    copier_(locator::locate<i_copier>()) {}

std::unique_ptr<candidate> candidate_factory::make(size_t idx) const {
    std::unordered_map<uint32_t, uint32_t> tm;
    const expr* copied_head = copier_.copy(db_.at(idx).head, tm);
    return std::make_unique<candidate>(copied_head, std::move(tm));
}
