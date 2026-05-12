#ifndef CANDIDATE_FACTORY_HPP
#define CANDIDATE_FACTORY_HPP

#include <memory>
#include "../domain/interfaces/i_factory.hpp"
#include "../domain/interfaces/i_candidate.hpp"
#include "../domain/interfaces/i_unifier.hpp"
#include "../domain/interfaces/i_translation_map.hpp"
#include "../domain/value_objects/lineage.hpp"

struct candidate_factory : i_factory<i_candidate, const resolution_lineage*> {
    candidate_factory(
        i_factory<i_unifier, const resolution_lineage*>&,
        i_factory<i_translation_map>&
    );
    std::unique_ptr<i_candidate> make(const resolution_lineage*) override;
private:
    i_factory<i_unifier, const resolution_lineage*>& unifier_factory_;
    i_factory<i_translation_map>& tm_factory_;
};

#endif
