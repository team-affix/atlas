#ifndef I_GET_CANDIDATE_TRANSLATION_MAP_HPP
#define I_GET_CANDIDATE_TRANSLATION_MAP_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/translation_map.hpp"

struct i_get_candidate_translation_map {
    virtual ~i_get_candidate_translation_map() = default;
    virtual translation_map& get(const resolution_lineage*) = 0;
};

#endif
