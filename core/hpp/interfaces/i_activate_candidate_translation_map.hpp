#ifndef I_ACTIVATE_CANDIDATE_TRANSLATION_MAP_HPP
#define I_ACTIVATE_CANDIDATE_TRANSLATION_MAP_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/translation_map.hpp"

struct i_activate_candidate_translation_map {
    virtual ~i_activate_candidate_translation_map() = default;
    virtual void activate(const resolution_lineage*, translation_map) = 0;
};

#endif
