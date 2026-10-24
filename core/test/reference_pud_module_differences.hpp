#ifndef REFERENCE_PUD_MODULE_DIFFERENCES_HPP
#define REFERENCE_PUD_MODULE_DIFFERENCES_HPP

#include <deque>
#include <string>
#include <vector>
#include "infrastructure/pud_module.hpp"

// Describes every way two lists of modules differ: ids, dependencies, axioms, and for every node
// reached from the axioms its refutation, its children and its inference. Empty when they hold the
// same database. Below a node whose children differ the walk does not descend.
inline std::vector<std::string> reference_pud_module_differences(
    const std::deque<pud_module>& expected, const std::deque<pud_module>& actual) {
    std::vector<std::string> differences;
    if (expected.size() != actual.size()) {
        differences.push_back("module count " + std::to_string(actual.size()) + " != " + std::to_string(expected.size()));
        return differences;
    }
    for (size_t index = 0; index < expected.size(); ++index) {
        const pud_module& want = expected.at(index);
        const pud_module& got = actual.at(index);
        const std::string where = "module " + std::to_string(want.get_module_id());
        if (got.get_module_id() != want.get_module_id()) {
            differences.push_back(where + ": id " + std::to_string(got.get_module_id()));
            continue;
        }
        if (got.get_dependencies() != want.get_dependencies())
            differences.push_back(where + ": dependencies");
        if (got.count_axioms() != want.count_axioms()) {
            differences.push_back(where + ": axiom count");
            continue;
        }
        std::vector<pud_node_id> pending;
        for (pud_node_id axiom = 0; axiom < want.count_axioms(); ++axiom) {
            if (!(got.get_axiom(axiom) == want.get_axiom(axiom)))
                differences.push_back(where + ": axiom " + std::to_string(axiom));
            pending.push_back(axiom);
        }
        while (!pending.empty()) {
            const pud_node_id node = pending.back();
            pending.pop_back();
            const std::string at = where + " node " + std::to_string(node);
            if (got.check_refuted(node) != want.check_refuted(node))
                differences.push_back(at + ": refutation");
            if (got.get_children(node) != want.get_children(node)) {
                differences.push_back(at + ": children");
                continue;
            }
            const bool inferred = node >= want.count_axioms();
            if (inferred && !(got.get_inference(node) == want.get_inference(node)))
                differences.push_back(at + ": inference");
            pending.insert(pending.end(), want.get_children(node).begin(), want.get_children(node).end());
        }
    }
    return differences;
}

#endif
