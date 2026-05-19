#ifndef I_DEACTIVATE_CANDIDATE_TRANSLATION_MAP_HPP
#define I_DEACTIVATE_CANDIDATE_TRANSLATION_MAP_HPP

#include "../value_objects/lineage.hpp"

struct i_deactivate_candidate_translation_map {
    virtual ~i_deactivate_candidate_translation_map() = default;
    virtual void deactivate(const resolution_lineage*) = 0;
};

#endif
