#ifndef PUD_NODE_ADDED_SUBSTITUTIONS_HPP
#define PUD_NODE_ADDED_SUBSTITUTIONS_HPP

#include <unordered_map>
#include <vector>
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_substitution.hpp"

struct pud_node_added_substitutions {
    const std::vector<pud_substitution>& get(pud_node_id id) const;
    void store(pud_node_id id, std::vector<pud_substitution> substitutions);
private:
    std::unordered_map<pud_node_id, std::vector<pud_substitution>> substitutions_;
};

#endif
