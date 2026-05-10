#ifndef CANDIDATE_NOT_APPLICABLE_DETECTOR_HPP
#define CANDIDATE_NOT_APPLICABLE_DETECTOR_HPP

#include <unordered_map>
#include <unordered_set>
#include "../interfaces/i_candidate_not_applicable_detector.hpp"
#include "../interfaces/i_unifier_frontier.hpp"
#include "../interfaces/i_expr_frontier.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_copier.hpp"
#include "../interfaces/i_expr_pool.hpp"
#include "../interfaces/i_translation_map_frontier.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/candidate_not_applicable_event.hpp"
#include "../value_objects/expr.hpp"

struct candidate_not_applicable_detector : i_candidate_not_applicable_detector {
    candidate_not_applicable_detector();

    void add_candidate(const resolution_lineage*) override;
    void remove_candidate(const resolution_lineage*) override;
    void primary_rep_changed(uint32_t var_index) override;
    void secondary_unify_failed(const resolution_lineage*) override;

private:
    void extract_vars(const expr*, std::unordered_set<uint32_t>&) const;

    std::unordered_map<uint32_t, std::unordered_set<const resolution_lineage*>> var_to_rls;
    std::unordered_map<const resolution_lineage*, std::unordered_set<uint32_t>> rl_to_vars;

    i_unifier_frontier& rbms;
    i_expr_frontier& ges;
    const i_database& db;
    i_copier& cp;
    i_expr_pool& ep;
    i_translation_map_frontier& rtms;
    i_event_producer<candidate_not_applicable_event>& candidate_not_applicable_producer;
};

#endif
