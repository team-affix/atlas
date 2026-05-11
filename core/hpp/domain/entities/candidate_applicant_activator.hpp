#ifndef CANDIDATE_APPLICANT_ACTIVATOR_HPP
#define CANDIDATE_APPLICANT_ACTIVATOR_HPP

#include "../interfaces/i_candidate_applicant_activator.hpp"
#include "../interfaces/i_applicant_frontier.hpp"
#include "../interfaces/i_factory.hpp"
#include "../interfaces/i_translation_map.hpp"
#include "../interfaces/i_unifier.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_copier.hpp"

struct candidate_applicant_activator : i_candidate_applicant_activator {
    candidate_applicant_activator();
    void activate(const resolution_lineage*) override;
private:
    i_applicant_frontier& af;
    i_factory<i_unifier, const resolution_lineage*>& u_factory;
    i_factory<i_translation_map>& tm_factory;
    i_database& db;
    i_copier& cp;
};

#endif
