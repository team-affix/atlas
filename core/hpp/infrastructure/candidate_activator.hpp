#ifndef CANDIDATE_ACTIVATOR_HPP
#define CANDIDATE_ACTIVATOR_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_candidate_activator.hpp"
#include "interfaces/i_frame_allocator.hpp"
#include "interfaces/i_set_candidate_frame_offset.hpp"
#include "interfaces/i_try_add_mhu_head.hpp"
#include "interfaces/i_is_backlogged_elimination.hpp"
#include "interfaces/i_get_goal_expr.hpp"
#include "interfaces/i_link_goal_candidate.hpp"
#include "interfaces/i_get_rule.hpp"

struct candidate_activator : i_candidate_activator {
    candidate_activator(locator& loc);
    void activate(const resolution_lineage*) override;
private:
    i_frame_allocator& frame_allocator;
    i_set_candidate_frame_offset& set_candidate_frame_offset;
    i_try_add_mhu_head& try_add_mhu_head;
    i_is_backlogged_elimination& is_backlogged_elimination;
    i_get_goal_expr& get_goal_expr;
    i_get_rule& get_rule;
    i_link_goal_candidate& link_goal_candidate;
};

#endif
