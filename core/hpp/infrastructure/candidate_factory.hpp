#ifndef CANDIDATE_FACTORY_HPP
#define CANDIDATE_FACTORY_HPP

#include <memory>
#include "../domain/interfaces/i_candidate_factory.hpp"
#include "../domain/interfaces/i_unifier.hpp"
#include "../domain/interfaces/i_translation_map.hpp"
#include "../domain/interfaces/i_database.hpp"
#include "../domain/interfaces/i_copier.hpp"

struct candidate_factory : i_candidate_factory {
    candidate_factory();
    std::unique_ptr<i_candidate> make(const resolution_lineage*, const expr*) override;
private:
    i_factory<i_unifier, const resolution_lineage*>& unifier_factory_;
    i_factory<i_translation_map>& tm_factory_;
    const i_database& db_;
    i_copier& copier_;
};

#endif
