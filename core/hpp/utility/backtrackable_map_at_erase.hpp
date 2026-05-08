#ifndef BACKTRACKABLE_MAP_AT_ERASE_HPP
#define BACKTRACKABLE_MAP_AT_ERASE_HPP

#include <cassert>
#include "i_backtrackable_mutation.hpp"

template<typename M>
struct backtrackable_map_at_erase : i_backtrackable_mutation<M> {
    backtrackable_map_at_erase(const M::key_type& key, const M::mapped_type::value_type& value);
    void invoke() override;
    void backtrack() override;
private:
    M::key_type key;
    M::mapped_type::value_type value;
};

template<typename M>
backtrackable_map_at_erase<M>::backtrackable_map_at_erase(const M::key_type& key, const M::mapped_type::value_type& value) :
    key(key),
    value(value) {
}

template<typename M>
void backtrackable_map_at_erase<M>::invoke() {
    auto erased = this->ref().at(key).erase(value);
    assert(erased == 1);
}

template<typename M>
void backtrackable_map_at_erase<M>::backtrack() {
    this->ref().at(key).insert(value);
}

#endif
