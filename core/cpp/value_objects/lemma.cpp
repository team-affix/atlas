#include "../../../hpp/domain/value_objects/lemma.hpp"

lemma::lemma(const std::unordered_set<const resolution_lineage*>& input) :
    rs(input) {

    std::unordered_set<const resolution_lineage*> visited;

    for (const resolution_lineage* rl : input)
        remove_ancestors(rl, visited);

}

const std::unordered_set<const resolution_lineage*>& lemma::get_resolutions() const {
    return rs;
}

void lemma::remove_ancestors(const resolution_lineage* rl, std::unordered_set<const resolution_lineage*>& visited) {
    while (rl) {
        const resolution_lineage* grandparent = rl->parent->parent;

        if (visited.contains(grandparent))
            break;

        visited.insert(grandparent);

        rs.erase(grandparent);

        rl = grandparent;
    }
}
