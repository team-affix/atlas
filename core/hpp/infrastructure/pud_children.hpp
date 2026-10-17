#ifndef PUD_CHILDREN_HPP
#define PUD_CHILDREN_HPP

#include <unordered_map>
#include <vector>
#include "value_objects/pud_node_id.hpp"

// A node's children are stored once and never change, so iterators into the list get() returns stay
// valid for good; witness and candidate searches keep such iterators across resumes and forks.
struct pud_children {
    const std::vector<pud_node_id>& get(pud_node_id id) const;
    void store(pud_node_id id, std::vector<pud_node_id> children);
private:
    static const std::vector<pud_node_id> empty_;
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> children_links_;
};

#endif
