#include "../../hpp/infrastructure/unifier_factory.hpp"
#include "../../hpp/domain/entities/unifier.hpp"

std::unique_ptr<i_unifier> unifier_factory::make(const resolution_lineage* rl) {
    return std::make_unique<unifier>(rl);
}
