#ifndef I_DERIVE_RESOLUTION_LEMMA_HPP
#define I_DERIVE_RESOLUTION_LEMMA_HPP

#include "../value_objects/lemma.hpp"

struct i_derive_resolution_lemma {
    virtual ~i_derive_resolution_lemma() = default;
    virtual lemma derive_resolution_lemma() const = 0;
};

#endif

