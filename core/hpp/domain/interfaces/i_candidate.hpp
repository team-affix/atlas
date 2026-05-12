#ifndef I_CANDIDATE_HPP
#define I_CANDIDATE_HPP

#include "i_translation_map.hpp"
#include "i_unifier.hpp"

struct i_candidate {
    virtual ~i_candidate() = default;
    virtual i_translation_map& tm() = 0;
    virtual i_unifier& u() = 0;
};

#endif
