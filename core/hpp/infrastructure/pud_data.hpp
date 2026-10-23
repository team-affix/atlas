#ifndef PUD_DATA_HPP
#define PUD_DATA_HPP

#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>
#include "value_objects/pud_node_delta.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/rule.hpp"

// The database tree, append-only: every axiom as its rule, every inference as its delta on top of its
// parent, and the children each node was unfolded into. A node's id is its position in the order nodes
// are stored. A node is a leaf until its first child is stored; every child of a node is resolved on
// the same call site.
struct pud_data {
    pud_data();
    pud_node_id store_axiom(const rule& axiom);
    pud_node_id store_inference(pud_node_delta delta);
    const rule& get_axiom(pud_node_id axiom) const;
    const pud_node_delta& get_inference(pud_node_id inference) const;
    std::optional<pud_node_id> get_parent(pud_node_id node) const;
    const std::vector<pud_node_id>& get_children(pud_node_id node) const;
    bool check_leaf(pud_node_id node) const;
private:
    std::unordered_map<pud_node_id, rule> axioms_;
    std::unordered_map<pud_node_id, pud_node_delta> inferences_;
    std::deque<std::vector<pud_node_id>> children_;
};

#endif
