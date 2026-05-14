#ifndef CANDIDATE_FACTORY_HPP
#define CANDIDATE_FACTORY_HPP

#include <memory>
#include "../domain/interfaces/i_factory.hpp"
#include "../domain/value_objects/candidate.hpp"

struct candidate_factory : i_factory<candidate, const resolution_lineage*> {
    candidate_factory();
    std::unique_ptr<candidate> make(const resolution_lineage*) const override;
};

#endif
