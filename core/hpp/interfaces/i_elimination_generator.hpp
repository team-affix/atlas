#ifndef I_ELIMINATION_GENERATOR_HPP
#define I_ELIMINATION_GENERATOR_HPP

#include "value_objects/lineage.hpp"
#include "infrastructure/coroutine.hpp"

class i_elimination_generator {
public:
    virtual ~i_elimination_generator() = default;
    virtual coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*) = 0;
};

#endif
