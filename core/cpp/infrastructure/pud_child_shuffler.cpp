#include "infrastructure/pud_child_shuffler.hpp"
#include <utility>

std::vector<pud_inference> pud_child_shuffler::shuffle(
    pud_global_node_id parent, std::vector<pud_inference> children) const {
    uint64_t state = parent.module ^ (static_cast<uint64_t>(parent.node) * 0x9e3779b97f4a7c15ull);
    for (size_t unshuffled = children.size(); unshuffled > 1; --unshuffled) {
        // The modulo's bias is negligible for the handful of children a node has.
        const size_t picked = static_cast<size_t>(next_random(state) % unshuffled);
        std::swap(children.at(picked), children.at(unshuffled - 1));
    }
    return children;
}

uint64_t pud_child_shuffler::next_random(uint64_t& state) {
    state += 0x9e3779b97f4a7c15ull;
    uint64_t mixed = state;
    mixed = (mixed ^ (mixed >> 30)) * 0xbf58476d1ce4e5b9ull;
    mixed = (mixed ^ (mixed >> 27)) * 0x94d049bb133111ebull;
    return mixed ^ (mixed >> 31);
}
