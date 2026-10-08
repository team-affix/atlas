#ifndef PUD_NODE_ADDED_SPECIALIZATIONS_HPP
#define PUD_NODE_ADDED_SPECIALIZATIONS_HPP

#include <unordered_map>
#include <vector>
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_specialization.hpp"

struct pud_node_added_specializations {
    const std::vector<pud_specialization>& get(pud_node_id id) const;
    void store(pud_node_id id, std::vector<pud_specialization> specs);
private:
    std::unordered_map<pud_node_id, std::vector<pud_specialization>> specs_;
};

#endif
