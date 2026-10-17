#ifndef PUD_AXIOMS_HPP
#define PUD_AXIOMS_HPP

#include <unordered_set>
#include <vector>
#include "value_objects/pud_node_id.hpp"

// The roots that make up the database. Goals only ever unfold onto these.
struct pud_axioms {
    void mark_axiom(pud_node_id id);
    const std::vector<pud_node_id>& get() const;
private:
    std::unordered_set<pud_node_id> axiom_set_;
    std::vector<pud_node_id> axioms_;
};

#endif
