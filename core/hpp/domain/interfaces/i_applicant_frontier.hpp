#ifndef I_APPLICANT_FRONTIER_HPP
#define I_APPLICANT_FRONTIER_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/applicant.hpp"
#include "i_map.hpp"

struct i_applicant_frontier : i_map<const resolution_lineage*, applicant> {
    virtual ~i_applicant_frontier() = default;
};

#endif
