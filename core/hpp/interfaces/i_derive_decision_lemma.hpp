#ifndef I_DERIVE_DECISION_LEMMA_HPP
#define I_DERIVE_DECISION_LEMMA_HPP

#include "value_objects/lemma.hpp"

struct i_derive_decision_lemma {
    virtual ~i_derive_decision_lemma() = default;
    virtual lemma derive_decision_lemma() const = 0;
};

#endif

