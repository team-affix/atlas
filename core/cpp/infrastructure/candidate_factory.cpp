#include "../../hpp/infrastructure/candidate_factory.hpp"
#include "../../hpp/domain/value_objects/candidate.hpp"

candidate_factory::candidate_factory() {
}

std::unique_ptr<candidate> candidate_factory::make(const resolution_lineage* rl) const {
    return std::make_unique<candidate>(rl);
}
