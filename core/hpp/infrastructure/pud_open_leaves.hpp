#ifndef PUD_OPEN_LEAVES_HPP
#define PUD_OPEN_LEAVES_HPP

#include <set>
#include "value_objects/pud_node_id.hpp"

// The live leaves that still have open body goals: exactly the leaves a decision may unfold.
struct pud_open_leaves {
    bool check_leaf_open(pud_node_id id) const;
    void mark_leaf_open(pud_node_id id);
    void mark_leaf_closed(pud_node_id id);
    const std::set<pud_node_id>& get() const;
private:
    std::set<pud_node_id> open_leaves_;
};

#endif
