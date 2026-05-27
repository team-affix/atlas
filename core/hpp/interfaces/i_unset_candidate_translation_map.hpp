#ifndef I_UNSET_CANDIDATE_TRANSLATION_MAP_HPP
#define I_UNSET_CANDIDATE_TRANSLATION_MAP_HPP

#include "../value_objects/lineage.hpp"

struct i_unset_candidate_translation_map {
    virtual ~i_unset_candidate_translation_map() = default;
    virtual void unset(const resolution_lineage*) = 0;
};

#endif
