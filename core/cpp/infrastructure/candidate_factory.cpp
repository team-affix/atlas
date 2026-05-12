#include "../../hpp/infrastructure/candidate_factory.hpp"
#include "../../hpp/domain/entities/candidate.hpp"

candidate_factory::candidate_factory(
    i_factory<i_unifier, const resolution_lineage*>& unifier_factory,
    i_factory<i_translation_map>& tm_factory)
    : unifier_factory_(unifier_factory), tm_factory_(tm_factory) {}

std::unique_ptr<i_candidate> candidate_factory::make(const resolution_lineage* rl) {
    return std::make_unique<candidate>(tm_factory_.make(), unifier_factory_.make(rl));
}
