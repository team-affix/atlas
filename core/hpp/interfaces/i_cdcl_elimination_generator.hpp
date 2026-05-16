#ifndef I_CDCL_ELIMINATION_GENERATOR_HPP
#define I_CDCL_ELIMINATION_GENERATOR_HPP

#include "i_elimination_generator.hpp"
#include "../value_objects/lineage.hpp"
#include "../value_objects/lemma.hpp"
#include "../utility/state_machine.hpp"

struct i_cdcl_elimination_generator : i_elimination_generator {
    virtual ~i_cdcl_elimination_generator() = default;
    virtual const resolution_lineage* learn(const lemma&) = 0;
};

#endif
