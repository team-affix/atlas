#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include "../interfaces/i_resolver.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_goal_activator.hpp"
#include "../interfaces/i_goal_deactivator.hpp"
#include "../interfaces/i_candidate_activator.hpp"
#include "../interfaces/i_goal_candidates_acceptor.hpp"
#include "../interfaces/i_goal_candidate_deactivator_visitor.hpp"
#include "../interfaces/i_conflict_detector.hpp"
#include "../interfaces/i_unit_goal_detector.hpp"
#include "../interfaces/i_unit_goals.hpp"

struct resolver : i_resolver {
    resolver(
        const i_database& db,
        i_lineage_pool& lp,
        i_goal_activator& goal_activator,
        i_goal_deactivator& goal_deactivator,
        i_candidate_activator& candidate_activator,
        i_goal_candidates_acceptor& gca,
        i_goal_candidate_deactivator_visitor& gcdv,
        i_conflict_detector& cd,
        i_unit_goal_detector& ugd,
        i_unit_goals& ug);
    bool resolve(const resolution_lineage*) override;
private:
    const i_database& db;
    i_lineage_pool& lp;
    i_goal_activator& goal_activator;
    i_goal_deactivator& goal_deactivator;
    i_candidate_activator& candidate_activator;
    i_goal_candidates_acceptor& gca;
    i_goal_candidate_deactivator_visitor& gcdv;
    i_conflict_detector& cd;
    i_unit_goal_detector& ugd;
    i_unit_goals& ug;
};

#endif
