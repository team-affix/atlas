#ifndef I_SET_CANDIDATE_TRANSLATION_MAP_HPP
#define I_SET_CANDIDATE_TRANSLATION_MAP_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/translation_map.hpp"

struct i_set_candidate_translation_map {
    virtual void set(const resolution_lineage*, translation_map) = 0;
};

#endif
