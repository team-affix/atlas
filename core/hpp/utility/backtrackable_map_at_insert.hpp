#ifndef BACKTRACKABLE_MAP_AT_INSERT_HPP
#define BACKTRACKABLE_MAP_AT_INSERT_HPP

#include <cassert>
#include "i_backtrackable_mutation.hpp"

template<typename M>
struct backtrackable_map_at_insert : i_backtrackable_mutation<M> {
    backtrackable_map_at_insert(const M::key_type& key, const M::mapped_type::value_type& value);
    void invoke() override;
    void backtrack() override;
private:
    M::key_type key;
    M::mapped_type::value_type value;
};

template<typename M>
backtrackable_map_at_insert<M>::backtrackable_map_at_insert(const M::key_type& key, const M::mapped_type::value_type& value) :
    key(key),
    value(value) {
}

template<typename M>
void backtrackable_map_at_insert<M>::invoke() {
    auto [_, inserted] = this->ref().at(key).insert(value);
    assert(inserted);
}

template<typename M>
void backtrackable_map_at_insert<M>::backtrack() {
    this->ref().at(key).erase(value);
}

#endif
