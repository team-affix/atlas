#ifndef I_DECISION_MEMORY_HPP
#define I_DECISION_MEMORY_HPP

#include "i_derive_decision_lemma.hpp"
#include "i_clear_decision_record.hpp"
#include "i_get_decision_count.hpp"
#include "i_record_decision.hpp"

struct i_decision_memory
    : i_record_decision
    , i_clear_decision_record
    , i_get_decision_count
    , i_derive_decision_lemma {
    virtual ~i_decision_memory() = default;
};

#endif
