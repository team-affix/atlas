#ifndef PUD_REFUTED_LEAVES_HPP
#define PUD_REFUTED_LEAVES_HPP

#include <unordered_set>
#include "value_objects/pud_node_id.hpp"

struct pud_refuted_leaves {
    pud_refuted_leaves();
    bool check_refuted(pud_node_id id) const;
    void mark_refuted(pud_node_id id);
private:
    std::unordered_set<pud_node_id> refuted_;
};

#endif
