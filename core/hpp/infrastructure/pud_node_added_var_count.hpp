#ifndef PUD_NODE_ADDED_VAR_COUNT_HPP
#define PUD_NODE_ADDED_VAR_COUNT_HPP

#include <cstdint>
#include <unordered_map>
#include "value_objects/pud_node_id.hpp"

struct pud_node_added_var_count {
    uint32_t get(pud_node_id id) const;
    void store(pud_node_id id, uint32_t var_count);
private:
    std::unordered_map<pud_node_id, uint32_t> var_counts_;
};

#endif
