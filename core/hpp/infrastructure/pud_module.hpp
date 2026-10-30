#ifndef PUD_MODULE_HPP
#define PUD_MODULE_HPP

#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <vector>
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/rule.hpp"

struct pud_module {
    pud_module(pud_module_id module_id, std::vector<pud_module_id> dependencies, std::vector<rule> axioms);
    pud_module_id get_module_id() const;
    const std::vector<pud_module_id>& get_dependencies() const;
    size_t count_axioms() const;
    size_t count_nodes() const;
    pud_node_id store_inference(pud_inference inference);
    void mark_refuted(pud_node_id node);
    const rule& get_axiom(pud_node_id axiom) const;
    const pud_inference& get_inference(pud_node_id inference) const;
    std::optional<pud_node_id> get_parent(pud_node_id node) const;
    const std::vector<pud_node_id>& get_children(pud_node_id node) const;
    bool check_leaf(pud_node_id node) const;
    bool check_refuted(pud_node_id node) const;
    const std::vector<std::string>& functor_names() const;
    uint32_t append_functor_name(const std::string& name);
private:
    pud_module_id module_id_;
    std::vector<pud_module_id> dependencies_;
    std::vector<rule> axioms_;
    std::deque<pud_inference> inferences_;
    std::deque<std::vector<pud_node_id>> children_;
    std::vector<bool> refuted_;
    std::vector<std::string> functor_names_;
};

#endif
