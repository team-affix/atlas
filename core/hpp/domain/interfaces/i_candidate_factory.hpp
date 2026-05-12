#ifndef I_CANDIDATE_FACTORY_HPP
#define I_CANDIDATE_FACTORY_HPP

#include "i_factory.hpp"
#include "i_candidate.hpp"
#include "../value_objects/lineage.hpp"
#include "../value_objects/expr.hpp"

struct i_candidate_factory : i_factory<i_candidate, const resolution_lineage*, const expr*> {
    virtual ~i_candidate_factory() = default;
};

#endif
