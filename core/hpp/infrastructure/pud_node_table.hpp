#ifndef PUD_NODE_TABLE_HPP
#define PUD_NODE_TABLE_HPP

#include <deque>
#include <optional>
#include <utility>
#include "value_objects/pud_node_id.hpp"
#include "debug_assert.hpp"

// One value per node, stored once. Node ids are dense, so values sit at their id; a deque keeps every
// stored value in place while later nodes are added.
template<typename T>
struct pud_node_table {
    pud_node_table();
    void store(pud_node_id id, T value);
    const T& get(pud_node_id id) const;
private:
    std::deque<std::optional<T>> values_;
};

template<typename T>
pud_node_table<T>::pud_node_table()
    : values_() {}

template<typename T>
void pud_node_table<T>::store(pud_node_id id, T value) {
    if (values_.size() <= id)
        values_.resize(id + 1);
    DEBUG_ASSERT(!values_.at(id).has_value());
    values_.at(id) = std::move(value);
}

template<typename T>
const T& pud_node_table<T>::get(pud_node_id id) const {
    return values_.at(id).value();
}

#endif
