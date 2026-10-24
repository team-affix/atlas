#ifndef PUD_MODULE_HPP
#define PUD_MODULE_HPP

#include <cstddef>
#include <deque>
#include <optional>
#include <vector>
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/rule.hpp"

// One module's tree, append-only: its axioms, given once at construction, every inference on top of
// its parent, the children each node was unfolded into, and which nodes are refuted. Nodes are
// numbered within the module in the order they are stored, the axioms first. A node is a leaf until
// its first child is stored; every child of a node is resolved on the same call site. A refuted node
// stays refuted.
struct pud_module {
    pud_module(pud_module_id module_id, std::vector<pud_module_id> dependencies, std::vector<rule> axioms);
    pud_module_id get_module_id() const;
    const std::vector<pud_module_id>& get_dependencies() const;
    size_t count_axioms() const;
    pud_node_id store_inference(pud_inference inference);
    void mark_refuted(pud_node_id node);
    const rule& get_axiom(pud_node_id axiom) const;
    const pud_inference& get_inference(pud_node_id inference) const;
    std::optional<pud_node_id> get_parent(pud_node_id node) const;
    const std::vector<pud_node_id>& get_children(pud_node_id node) const;
    bool check_leaf(pud_node_id node) const;
    bool check_refuted(pud_node_id node) const;
private:
    pud_module_id module_id_;
    std::vector<pud_module_id> dependencies_;
    std::vector<rule> axioms_;
    std::deque<pud_inference> inferences_;
    std::deque<std::vector<pud_node_id>> children_;
    std::vector<bool> refuted_;
};

#endif
