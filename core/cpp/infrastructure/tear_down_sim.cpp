#include "infrastructure/tear_down_sim.hpp"

tear_down_sim::tear_down_sim(locator& loc)
    : pop_trail_frame_(loc.locate<i_pop_trail_frame>()),
      clear_unit_goals_(loc.locate<i_clear_unit_goals>()),
      clear_recorded_decisions_(loc.locate<i_clear_recorded_decisions>()),
      clear_recorded_resolutions_(loc.locate<i_clear_recorded_resolutions>()),
      clear_goal_candidate_rule_ids_(loc.locate<i_clear_goal_candidate_rule_ids>()),
      clear_goal_exprs_(loc.locate<i_clear_goal_exprs>()),
      clear_active_goals_(loc.locate<i_clear_active_goals>()),
      clear_candidate_frame_offsets_(loc.locate<i_clear_candidate_frame_offsets>()),
      clear_mhu_heads_(loc.locate<i_clear_mhu_heads>()),
      clear_bindings_(loc.locate<i_clear_bindings>()),
      trim_unpinned_lineages_(loc.locate<i_trim_unpinned_lineages>()),
      frame_allocator_(loc.locate<i_frame_allocator>()),
      clean_up_cdcl_(loc.locate<i_clean_up_cdcl>()),
      clear_chosen_goal_candidates_(loc.locate<i_clear_chosen_goal_candidates>()) {}

void tear_down_sim::tear_down() {
    pop_trail_frame_.pop();
    clear_unit_goals_.clear();
    clear_recorded_decisions_.clear_recorded_decisions();
    clear_recorded_resolutions_.clear_recorded_resolutions();
    clear_goal_candidate_rule_ids_.clear_goal_candidate_rule_ids();
    clear_goal_exprs_.clear_goal_exprs();
    clear_active_goals_.clear_active_goals();
    clear_candidate_frame_offsets_.clear_candidate_frame_offsets();
    clear_mhu_heads_.clear_mhu_heads();
    clear_bindings_.clear_bindings();
    frame_allocator_.reset();
    clean_up_cdcl_.cleanup();
    clear_chosen_goal_candidates_.clear();
    trim_unpinned_lineages_.trim();
}
