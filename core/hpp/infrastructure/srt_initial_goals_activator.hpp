#ifndef SRT_INITIAL_GOALS_ACTIVATOR_HPP
#define SRT_INITIAL_GOALS_ACTIVATOR_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_srt_flush_goal_batch.hpp"
#include "infrastructure/initial_goals_activator.hpp"

struct srt_initial_goals_activator : i_activate_initial_goals_and_candidates {
    srt_initial_goals_activator(locator& loc);
    bool activate_initial_goals_and_candidates() override;
private:
    i_srt_flush_goal_batch& flush_srt_goal_batch_;
    initial_goals_activator& initial_goals_activator_;
};

#endif
