#ifndef PUD_CHILDREN_HPP
#define PUD_CHILDREN_HPP

#include <unordered_map>
#include <vector>
#include "value_objects/pud_node_id.hpp"

struct pud_children {
    const std::vector<pud_node_id>& get(pud_node_id id) const;
    void store(pud_node_id id, std::vector<pud_node_id> children);
private:
    static const std::vector<pud_node_id> empty_;
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> children_links_;
};

#endif
