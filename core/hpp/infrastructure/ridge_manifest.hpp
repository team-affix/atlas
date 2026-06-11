#ifndef RIDGE_MANIFEST_HPP
#define RIDGE_MANIFEST_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include "infrastructure/srt_active_goals.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/rule_id_set_factory.hpp"
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
#include "infrastructure/elimination_backlog.hpp"
#include "infrastructure/elimination_router.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/get_resolution_rule.hpp"
#include "infrastructure/get_unit_resolution.hpp"
#include "infrastructure/goal_activator.hpp"
#include "infrastructure/goal_candidates_activator.hpp"
#include "infrastructure/goal_candidates_deactivator.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/srt_goal_deactivator.hpp"
#include "infrastructure/initial_goals_activator.hpp"
#include "infrastructure/srt_initial_goals_activator.hpp"
#include "infrastructure/goal_exprs.hpp"
#include "infrastructure/initial_goal_activator.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/joint_elimination_generator.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/locator.hpp"
#include "infrastructure/make_initial_goal_lineage.hpp"
#include "infrastructure/mhu_elimination_generator.hpp"
#include "infrastructure/mcts_decision_generator.hpp"
#include "infrastructure/mcts_sim.hpp"
#include "infrastructure/ridge_reward.hpp"
#include "infrastructure/resolution_memory.hpp"
#include "infrastructure/resolver.hpp"
#include "infrastructure/subgoals_activator.hpp"
#include "infrastructure/srt_subgoals_activator.hpp"
#include "infrastructure/set_up_sim.hpp"
#include "infrastructure/tear_down_sim.hpp"
#include "infrastructure/run_sim.hpp"
#include "infrastructure/solution_detector.hpp"
#include "infrastructure/solver.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/unit_goal_detector.hpp"
#include "infrastructure/unit_goals.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/cdcl_sequencer.hpp"
#include "infrastructure/var_sequencer.hpp"

struct ridge_manifest {
    ridge_manifest(
        db& database,
        initial_goal_exprs& initial_goals,
        size_t initial_var_count,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant);

    locator loc_;
    db& database_;
    initial_goal_exprs& initial_goals_;
    size_t max_resolutions_;

    trail& trail_;
    bind_map& bind_map_;
    bind_map_factory& bind_map_factory_;
    unifier_factory& unifier_factory_;
    expr_pool& expr_pool_;
    var_sequencer& var_sequencer_;
    cdcl_sequencer& cdcl_sequencer_;
    lineage_pool& lineage_pool_;
    srt_active_goals& srt_active_goals_;
    goal_exprs& goal_exprs_;
    goal_candidate_rules& goal_candidate_rules_;
    unit_goals& unit_goals_;
    decision_memory& decision_memory_;
    resolution_memory& resolution_memory_;
    elimination_backlog& elimination_backlog_;
    cdcl_elimination_generator& cdcl_;
    mhu_elimination_generator& mhu_;
    joint_elimination_generator& joint_;
    candidate_translation_maps& candidate_translation_maps_;
    get_resolution_rule& get_resolution_rule_;
    copier& copier_;
    conflict_detector& conflict_detector_;
    unit_goal_detector& unit_goal_detector_;
    solution_detector& solution_detector_;
    goal_activator& goal_activator_;
    srt_goal_deactivator& srt_goal_deactivator_;
    candidate_activator& candidate_activator_;
    candidate_deactivator& candidate_deactivator_;
    elimination_router& elimination_router_;
    get_unit_resolution& get_unit_resolution_;
    make_initial_goal_lineage& make_initial_goal_lineage_;
    initial_goal_activator& initial_goal_activator_;
    std::mt19937& rng_;
    mcts_decision_generator& mcts_decision_generator_;
    resolver& resolver_;
    set_up_sim& set_up_sim_;
    tear_down_sim& tear_down_sim_;
    run_sim& run_sim_;
    ridge_reward& ridge_reward_;
    mcts_sim& mcts_sim_;
    solver& solver_;

private:
    struct early_wiring {
        trail trail_;
        bind_map bind_map_;
        bind_map_factory bind_map_factory_;
        unifier_factory unifier_factory_;
        lineage_pool lineage_pool_;
        rule_id_set_factory rule_id_set_factory_;
        ra_rule_id_set_factory ra_rule_id_set_factory_;
        srt_active_goals srt_active_goals_;
        goal_exprs goal_exprs_;
        goal_candidate_rules goal_candidate_rules_;
        unit_goals unit_goals_;
        decision_memory decision_memory_;
        resolution_memory resolution_memory_;
        candidate_translation_maps candidate_translation_maps_;

        early_wiring(locator& loc, db& database, initial_goal_exprs& initial_goals);
    };

    struct pool_wiring {
        expr_pool expr_pool_;
        var_sequencer var_sequencer_;
        cdcl_sequencer cdcl_sequencer_;
        elimination_backlog elimination_backlog_;

        pool_wiring(locator& loc, size_t initial_var_count);
    };

    struct elim_wiring {
        cdcl_elimination_generator cdcl_;
        mhu_elimination_generator mhu_;
        std::optional<joint_elimination_generator> joint_;

        elim_wiring(locator& loc);
    };

    struct core_wiring {
        get_resolution_rule get_resolution_rule_;
        copier copier_;
        conflict_detector conflict_detector_;
        unit_goal_detector unit_goal_detector_;
        solution_detector solution_detector_;

        core_wiring(locator& loc);
    };

    struct activator_wiring {
        goal_activator goal_activator_;
        srt_goal_deactivator srt_goal_deactivator_;
        candidate_activator candidate_activator_;
        candidate_deactivator candidate_deactivator_;

        activator_wiring(locator& loc);
    };

    struct router_wiring {
        elimination_router elimination_router_;
        get_unit_resolution get_unit_resolution_;
        make_initial_goal_lineage make_initial_goal_lineage_;
        std::optional<initial_goal_activator> initial_goal_activator_;

        router_wiring(locator& loc);
    };

    struct orchestration_wiring {
        goal_candidates_activator goal_candidates_activator_;
        goal_candidates_deactivator goal_candidates_deactivator_;
        std::optional<subgoals_activator> subgoals_activator_;
        std::optional<initial_goals_activator> initial_goals_activator_;
        std::optional<srt_subgoals_activator> srt_subgoals_activator_;
        std::optional<srt_initial_goals_activator> srt_initial_goals_activator_;
        std::mt19937 rng_;
        std::optional<set_up_sim> set_up_sim_;
        std::optional<tear_down_sim> tear_down_sim_;
        std::optional<resolver> resolver_;
        ridge_reward ridge_reward_;
        std::optional<mcts_sim> mcts_sim_;
        std::optional<mcts_decision_generator> mcts_decision_generator_;
        std::optional<run_sim> run_sim_;
        std::optional<solver> solver_;

        orchestration_wiring(locator& loc, size_t max_resolutions, uint32_t random_seed,
            double exploration_constant);
    };

    early_wiring early_;
    pool_wiring pools_;
    elim_wiring elims_;
    core_wiring core_;
    activator_wiring activators_;
    router_wiring routers_;
    orchestration_wiring orch_;
};

#endif
