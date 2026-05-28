#ifndef BASIC_MANIFEST_HPP
#define BASIC_MANIFEST_HPP

#include <cstddef>
#include <cstdint>
#include <random>

#include "infrastructure/active_goals.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/candidate_activator.hpp"
#include "infrastructure/candidate_deactivator.hpp"
#include "infrastructure/candidate_translation_maps.hpp"
#include "infrastructure/cdcl_elimination_generator.hpp"
#include "infrastructure/conflict_detector.hpp"
#include "infrastructure/copier.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/decision_memory.hpp"
#include "infrastructure/deactivated_candidate_memory.hpp"
#include "infrastructure/elimination_backlog.hpp"
#include "infrastructure/elimination_router.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/get_resolution_rule.hpp"
#include "infrastructure/get_unit_resolution.hpp"
#include "infrastructure/goal_activator.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/goal_deactivator.hpp"
#include "infrastructure/goal_exprs.hpp"
#include "infrastructure/initial_goal_activator.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/joint_elimination_generator.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/make_initial_goal_lineage.hpp"
#include "infrastructure/mhu_elimination_generator.hpp"
#include "infrastructure/overlay_bind_map_factory.hpp"
#include "infrastructure/random_decision_generator.hpp"
#include "infrastructure/resolution_memory.hpp"
#include "infrastructure/resolver.hpp"
#include "infrastructure/sim.hpp"
#include "infrastructure/solution_detector.hpp"
#include "infrastructure/solver.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/unit_goal_detector.hpp"
#include "infrastructure/unit_goals.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/var_sequencer.hpp"

struct basic_manifest {
    basic_manifest(
        db& database,
        initial_goal_exprs& initial_goals,
        size_t max_resolutions,
        uint32_t random_seed = 0);

    db& database_;
    initial_goal_exprs& initial_goals_;

    trail trail_;
    bind_map bind_map_;
    bind_map_factory bind_map_factory_;
    overlay_bind_map_factory overlay_bind_map_factory_;
    unifier_factory unifier_factory_;
    expr_pool expr_pool_;
    var_sequencer var_sequencer_;
    lineage_pool lineage_pool_;
    active_goals active_goals_;
    goal_exprs goal_exprs_;
    goal_candidate_rules goal_candidate_rules_;
    unit_goals unit_goals_;
    decision_memory decision_memory_;
    resolution_memory resolution_memory_;
    deactivated_candidate_memory deactivated_candidate_memory_;
    elimination_backlog elimination_backlog_;
    cdcl_elimination_generator cdcl_;
    mhu_elimination_generator mhu_;
    joint_elimination_generator joint_;
    candidate_translation_maps candidate_translation_maps_;
    get_resolution_rule get_resolution_rule_;
    copier copier_;
    conflict_detector conflict_detector_;
    unit_goal_detector unit_goal_detector_;
    solution_detector solution_detector_;
    goal_activator goal_activator_;
    goal_deactivator goal_deactivator_;
    candidate_activator candidate_activator_;
    candidate_deactivator candidate_deactivator_;
    elimination_router elimination_router_;
    get_unit_resolution get_unit_resolution_;
    make_initial_goal_lineage make_initial_goal_lineage_;
    initial_goal_activator initial_goal_activator_;
    std::mt19937 rng_;
    random_decision_generator random_decision_generator_;
    resolver resolver_;
    sim sim_;
    solver solver_;
};

#endif
