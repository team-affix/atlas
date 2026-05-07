#ifndef BACKTRACKABLE_MAP_ASSIGN_HPP
#define BACKTRACKABLE_MAP_ASSIGN_HPP

#include <utility>
#include "i_backtrackable_mutation.hpp"

template<typename M>
struct backtrackable_map_assign : i_backtrackable_mutation<M> {
    backtrackable_map_assign(const M::key_type& key, const M::mapped_type& new_value);
    void invoke() override;
    void backtrack() override;
private:
    M::key_type key;
    M::mapped_type value;
};

template<typename M>
backtrackable_map_assign<M>::backtrackable_map_assign(const M::key_type& key, const M::mapped_type& value) : key(key), value(value) {
}

template<typename M>
void backtrackable_map_assign<M>::invoke() {
    std::swap(this->ref().at(key), value);
}

template<typename M>
void backtrackable_map_assign<M>::backtrack() {
    std::swap(this->ref().at(key), value);
}

#endif
