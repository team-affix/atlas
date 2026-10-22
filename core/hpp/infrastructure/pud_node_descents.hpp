#ifndef PUD_NODE_DESCENTS_HPP
#define PUD_NODE_DESCENTS_HPP

#include <unordered_map>
#include <utility>
#include "value_objects/pud_node_id.hpp"
#include "debug_assert.hpp"

// Keeps the descent of every live leaf, and with it the queries that descent owns.
template<typename Descent>
struct pud_node_descents {
    bool check_descent(pud_node_id id) const;
    const Descent& get(pud_node_id id) const;
    void store(pud_node_id id, Descent descent);
    void erase(pud_node_id id);
private:
    std::unordered_map<pud_node_id, Descent> descents_;
};

template<typename Descent>
bool pud_node_descents<Descent>::check_descent(pud_node_id id) const {
    return descents_.contains(id);
}

template<typename Descent>
const Descent& pud_node_descents<Descent>::get(pud_node_id id) const {
    return descents_.at(id);
}

template<typename Descent>
void pud_node_descents<Descent>::store(pud_node_id id, Descent descent) {
    auto [_, inserted] = descents_.emplace(id, std::move(descent));
    DEBUG_ASSERT(inserted);
}

template<typename Descent>
void pud_node_descents<Descent>::erase(pud_node_id id) {
    descents_.erase(id);
}

#endif
