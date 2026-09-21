#ifndef PUD_NODE_LVC_HPP
#define PUD_NODE_LVC_HPP

#include <cstdint>
#include <unordered_map>
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

struct pud_node_lvc {
    pud_node_lvc();
    void store(const pud_rule_id* id, uint32_t lvc);
    uint32_t get(const pud_rule_id* id) const;
private:
    using map_t = std::unordered_map<const pud_rule_id*, uint32_t>;

    map_t by_id_;
};

#endif
