#ifndef BACKTRACKABLE_MAP_ERASE_HPP
#define BACKTRACKABLE_MAP_ERASE_HPP

#include <utility>
#include <cassert>
#include "i_backtrackable_mutation.hpp"

template<typename M>
struct backtrackable_map_erase : i_backtrackable_mutation<M> {
    backtrackable_map_erase(const M::key_type& key);
    void invoke() override;
    void backtrack() override;
private:
    M::key_type key;
    M::mapped_type value;
};

template<typename M>
backtrackable_map_erase<M>::backtrackable_map_erase(const M::key_type& key) : key(key) {
}

template<typename M>
void backtrackable_map_erase<M>::invoke() {
    value = std::move(this->ref().at(key));
    auto erased = this->ref().erase(key);
    assert(erased == 1);
}

template<typename M>
void backtrackable_map_erase<M>::backtrack() {
    this->ref().insert({key, std::move(value)});
}

#endif
