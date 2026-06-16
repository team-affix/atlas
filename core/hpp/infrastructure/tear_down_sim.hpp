#ifndef TEAR_DOWN_SIM_HPP
#define TEAR_DOWN_SIM_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_tear_down_sim.hpp"
#include "interfaces/i_pop_trail_frame.hpp"
#include "interfaces/i_clear_unit_goals.hpp"
#include "interfaces/i_clear_recorded_decisions.hpp"
#include "interfaces/i_clear_recorded_resolutions.hpp"
#include "interfaces/i_clear_goal_candidate_rule_ids.hpp"
#include "interfaces/i_clear_goal_exprs.hpp"
#include "interfaces/i_clear_active_goals.hpp"
#include "interfaces/i_clear_candidate_frame_offsets.hpp"
#include "interfaces/i_clear_mhu_heads.hpp"
#include "interfaces/i_clear_bindings.hpp"
#include "interfaces/i_trim_unpinned_lineages.hpp"
#include "interfaces/i_frame_allocator.hpp"

struct tear_down_sim : i_tear_down_sim {
    tear_down_sim(locator& loc);
    void tear_down() override;
private:
    i_pop_trail_frame& pop_trail_frame_;
    i_clear_unit_goals& clear_unit_goals_;
    i_clear_recorded_decisions& clear_recorded_decisions_;
    i_clear_recorded_resolutions& clear_recorded_resolutions_;
    i_clear_goal_candidate_rule_ids& clear_goal_candidate_rule_ids_;
    i_clear_goal_exprs& clear_goal_exprs_;
    i_clear_active_goals& clear_active_goals_;
    i_clear_candidate_frame_offsets& clear_candidate_frame_offsets_;
    i_clear_mhu_heads& clear_mhu_heads_;
    i_clear_bindings& clear_bindings_;
    i_trim_unpinned_lineages& trim_unpinned_lineages_;
    i_frame_allocator& frame_allocator_;
};

#endif
