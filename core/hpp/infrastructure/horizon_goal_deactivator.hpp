#ifndef HORIZON_GOAL_DEACTIVATOR_HPP
#define HORIZON_GOAL_DEACTIVATOR_HPP

#include "infrastructure/locator.hpp"
#include "infrastructure/srt_goal_deactivator.hpp"
#include "interfaces/i_goal_deactivator.hpp"
#include "interfaces/i_erase_goal_weight.hpp"

struct horizon_goal_deactivator : i_goal_deactivator {
    horizon_goal_deactivator(locator& loc);
    void deactivate(const goal_lineage*) override;
private:
    srt_goal_deactivator& srt_goal_deactivator_;
    i_erase_goal_weight& erase_goal_weight_;
};

#endif
