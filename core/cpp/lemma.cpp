#include "../hpp/lemma.hpp"


lemma::lemma(const resolutions& rs) : value(rs) {
    // 1. create sets of visited lineages
    std::set<const resolution_lineage*> visited;

    // 3. iterate over ds, and for each entry, remove all ancestors from av
    for (const resolution_lineage* rl : value)
        remove_ancestors(rl, visited);
}

const resolutions& lemma::get() const {
    return value;
}

void lemma::remove_ancestors(const resolution_lineage* rl, std::set<const resolution_lineage*>& visited) {
    while (rl) {
        // 1. get grandparent
        //    (double-dereference safe because resolutions should
        //     never have null parent goals)
        const resolution_lineage* grandparent = rl->parent->parent;
        
        // 2. check grandparent visited
        if (visited.contains(grandparent))
            break;

        // 3. visit grandparent
        visited.insert(grandparent);

        // 4. remove grandparent from av
        value.erase(grandparent);

        // 5. rl = grandparent
        rl = grandparent;

    }
}
