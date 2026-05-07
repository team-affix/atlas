#ifndef BACKTRACKABLE_MAP_INSERT_HPP
#define BACKTRACKABLE_MAP_INSERT_HPP

#include <utility>
#include <cassert>
#include "i_backtrackable_mutation.hpp"

template<typename M>
struct backtrackable_map_insert : i_backtrackable_mutation<M> {
    backtrackable_map_insert(const M::key_type& key, const M::mapped_type& value);
    void invoke() override;
    void backtrack() override;
private:
    M::key_type key;
    M::mapped_type value;
};

template<typename M>
backtrackable_map_insert<M>::backtrackable_map_insert(const M::key_type& key, const M::mapped_type& value) : key(key), value(value) {
}

template<typename M>
void backtrackable_map_insert<M>::invoke() {
    auto [_, inserted] = this->ref().insert({key, std::move(value)});
    assert(inserted);
}

template<typename M>
void backtrackable_map_insert<M>::backtrack() {
    this->ref().erase(key);
}

#endif
