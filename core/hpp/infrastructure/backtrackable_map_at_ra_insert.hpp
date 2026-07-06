#ifndef BACKTRACKABLE_MAP_AT_RA_INSERT_HPP
#define BACKTRACKABLE_MAP_AT_RA_INSERT_HPP

#include "infrastructure/backtrackable_mutation.hpp"

// Inserts a value into the inner set at M::at(key), undoing with erase. Unlike
// backtrackable_map_at_insert this targets an inner container whose insert/erase
// return void and assert internally (e.g. ra_rule_id_set). Capturing the outer
// map and re-looking up the key on backtrack keeps the undo valid even if the
// outer node is erased and re-inserted (at a new address) in the same frame.
template<typename M>
struct backtrackable_map_at_ra_insert : backtrackable_mutation<M> {
    backtrackable_map_at_ra_insert(const M::key_type& key, const M::mapped_type::value_type& value);
    void invoke() override;
    void backtrack() override;
private:
    M::key_type key;
    M::mapped_type::value_type value;
};

template<typename M>
backtrackable_map_at_ra_insert<M>::backtrackable_map_at_ra_insert(const M::key_type& key, const M::mapped_type::value_type& value) :
    key(key),
    value(value) {
}

template<typename M>
void backtrackable_map_at_ra_insert<M>::invoke() {
    this->ref().at(key).insert(value);
}

template<typename M>
void backtrackable_map_at_ra_insert<M>::backtrack() {
    this->ref().at(key).erase(value);
}

#endif
