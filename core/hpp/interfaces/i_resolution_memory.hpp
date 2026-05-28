#ifndef I_RESOLUTION_MEMORY_HPP
#define I_RESOLUTION_MEMORY_HPP

#include "i_clear_resolution_record.hpp"
#include "i_derive_resolution_lemma.hpp"
#include "i_get_resolution_count.hpp"
#include "i_record_resolution.hpp"

struct i_resolution_memory
    : i_record_resolution
    , i_clear_resolution_record
    , i_get_resolution_count
    , i_derive_resolution_lemma {
    virtual ~i_resolution_memory() = default;
};

#endif
