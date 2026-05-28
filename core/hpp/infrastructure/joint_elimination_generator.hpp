#ifndef JOINT_ELIMINATION_GENERATOR_HPP
#define JOINT_ELIMINATION_GENERATOR_HPP

#include "../interfaces/i_elimination_generator.hpp"

struct joint_elimination_generator : i_elimination_generator {
    joint_elimination_generator(
        i_elimination_generator& cdcl,
        i_elimination_generator& mhu);
    state_machine<const resolution_lineage*> constrain(const resolution_lineage*) override;
private:
    i_elimination_generator& cdcl;
    i_elimination_generator& mhu;
};

#endif
