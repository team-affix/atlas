#ifndef PUD_NODE_ADDED_TOUCHED_CALLER_REPS_HPP
#define PUD_NODE_ADDED_TOUCHED_CALLER_REPS_HPP

#include <cstdint>
#include <unordered_map>
#include <vector>
#include "value_objects/pud_rule_id.hpp"

struct pud_node_added_touched_caller_reps {
    pud_node_added_touched_caller_reps();
    void store(const pud_rule_id* id, std::vector<uint32_t> reps);
    const std::vector<uint32_t>& get(const pud_rule_id* id) const;
private:
    using map_t = std::unordered_map<const pud_rule_id*, std::vector<uint32_t>>;

    map_t by_id_;
};

#endif
