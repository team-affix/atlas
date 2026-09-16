#ifndef PUD_RULE_ID_HASH_HPP
#define PUD_RULE_ID_HASH_HPP

#include <cstddef>
#include "pud_rule_id.hpp"

struct pud_rule_id_hash {
    size_t operator()(const pud_rule_id& id) const noexcept;

private:
    static size_t hash_combine(size_t seed, size_t value) noexcept;
};

#endif
