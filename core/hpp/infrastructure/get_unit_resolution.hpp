#ifndef GET_UNIT_RESOLUTION_HPP
#define GET_UNIT_RESOLUTION_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_get_unit_resolution.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"

struct get_unit_resolution : i_get_unit_resolution {
    get_unit_resolution(locator& loc);
    const resolution_lineage* get(const goal_lineage*) override;
private:
    i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids;
    i_make_resolution_lineage& make_resolution_lineage;
};

#endif
