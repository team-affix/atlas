#ifndef PUD_NODE_TABLE_HPP
#define PUD_NODE_TABLE_HPP

#include <unordered_map>
#include <utility>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_global_node_id_hash.hpp"
#include "debug_assert.hpp"

// One value per node, stored once. Every stored value stays in place while later nodes are added.
template<typename T>
struct pud_node_table {
    pud_node_table();
    void store(pud_global_node_id id, T value);
    const T& get(pud_global_node_id id) const;
private:
    std::unordered_map<pud_global_node_id, T, pud_global_node_id_hash> values_;
};

template<typename T>
pud_node_table<T>::pud_node_table()
    : values_() {}

template<typename T>
void pud_node_table<T>::store(pud_global_node_id id, T value) {
    [[maybe_unused]] const bool stored = values_.emplace(id, std::move(value)).second;
    DEBUG_ASSERT(stored);
}

template<typename T>
const T& pud_node_table<T>::get(pud_global_node_id id) const {
    return values_.at(id);
}

#endif
