#ifndef PUD_CANDIDATE_SELF_WITNESS_HASH_HPP
#define PUD_CANDIDATE_SELF_WITNESS_HASH_HPP

#include <cstddef>
#include "value_objects/pud_candidate_self_witness.hpp"

struct pud_candidate_self_witness_hash {
    size_t operator()(const pud_candidate_self_witness& context) const noexcept;
};

#endif
