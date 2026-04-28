#include "../../../hpp/domain/data_structures/lemma.hpp"

lemma::lemma(const resolutions& input) :
    rs(input) {

    std::set<const resolution_lineage*> visited;

    for (const resolution_lineage* rl : input)
        remove_ancestors(rl, visited);

}

const resolutions& lemma::get_resolutions() const {
    return rs;
}

void lemma::remove_ancestors(const resolution_lineage* rl, std::set<const resolution_lineage*>& visited) {
    while (rl) {
        const resolution_lineage* grandparent = rl->parent->parent;

        if (visited.contains(grandparent))
            break;

        visited.insert(grandparent);

        rs.erase(grandparent);

        rl = grandparent;
    }
}
