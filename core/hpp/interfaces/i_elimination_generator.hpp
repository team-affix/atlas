#ifndef I_ELIMINATION_GENERATOR_HPP
#define I_ELIMINATION_GENERATOR_HPP

#include "../value_objects/lineage.hpp"
#include "../utility/state_machine.hpp"

class i_elimination_generator {
public:
    virtual ~i_elimination_generator() = default;
    virtual state_machine<const resolution_lineage*> constrain(const resolution_lineage*) = 0;
};

#endif
