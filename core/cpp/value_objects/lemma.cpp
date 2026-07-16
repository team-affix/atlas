#include "value_objects/lemma.hpp"

lemma::lemma(const std::unordered_set<const resolution_lineage*>& input)
    : rs_(input) {

    std::unordered_set<const resolution_lineage*> visited;

    for (const resolution_lineage* rl : input)
        remove_ancestors(rl, visited);

}

const std::unordered_set<const resolution_lineage*>& lemma::get_resolutions() const {
    return rs_;
}

void lemma::remove_ancestors(const resolution_lineage* rl, std::unordered_set<const resolution_lineage*>& visited) {
    while (rl) {
        if (!rl->parent || !rl->parent->parent)
            break;
        const resolution_lineage* grandparent = rl->parent->parent;

        if (visited.contains(grandparent))
            break;

        visited.insert(grandparent);

        rs_.erase(grandparent);

        rl = grandparent;
    }
}
