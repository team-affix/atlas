#ifndef PUD_NODE_HEADS_HPP
#define PUD_NODE_HEADS_HPP

#include <unordered_map>
#include "value_objects/pud_node_id.hpp"
#include "value_objects/expr.hpp"

struct pud_node_heads {
    const expr* get(pud_node_id id) const;
    void store(pud_node_id id, const expr* head);
private:
    std::unordered_map<pud_node_id, const expr*> heads_;
};

#endif
