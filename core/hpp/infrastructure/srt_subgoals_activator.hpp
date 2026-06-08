#ifndef SRT_SUBGOALS_ACTIVATOR_HPP
#define SRT_SUBGOALS_ACTIVATOR_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_activate_subgoals_and_candidates.hpp"
#include "interfaces/i_srt_link_goal_batch_parent.hpp"
#include "interfaces/i_srt_flush_goal_batch.hpp"
#include "infrastructure/subgoals_activator.hpp"

struct srt_subgoals_activator : i_activate_subgoals_and_candidates {
    srt_subgoals_activator(locator& loc);
    bool activate_subgoals_and_candidates(const resolution_lineage*) override;
private:
    i_srt_link_goal_batch_parent& link_srt_goal_batch_parent_;
    i_srt_flush_goal_batch& flush_srt_goal_batch_;
    subgoals_activator& subgoals_activator_;
};

#endif
