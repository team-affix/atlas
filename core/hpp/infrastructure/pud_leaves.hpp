#ifndef PUD_LEAVES_HPP
#define PUD_LEAVES_HPP

#include <unordered_set>
#include "value_objects/pud_node_id.hpp"

struct pud_leaves {
    bool check_leaf(pud_node_id id) const;
    void set_leaf(pud_node_id id);
    void unset_leaf(pud_node_id id);
private:
    std::unordered_set<pud_node_id> leaves_;
};

#endif
