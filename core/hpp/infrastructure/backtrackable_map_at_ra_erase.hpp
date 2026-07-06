#ifndef BACKTRACKABLE_MAP_AT_RA_ERASE_HPP
#define BACKTRACKABLE_MAP_AT_RA_ERASE_HPP

#include "infrastructure/backtrackable_mutation.hpp"

// Erases a value from the inner set at M::at(key), undoing with insert. Inner
// container counterpart of backtrackable_map_at_ra_insert (void insert/erase
// that assert internally, e.g. ra_rule_id_set).
template<typename M>
struct backtrackable_map_at_ra_erase : backtrackable_mutation<M> {
    backtrackable_map_at_ra_erase(const M::key_type& key, const M::mapped_type::value_type& value);
    void invoke() override;
    void backtrack() override;
private:
    M::key_type key;
    M::mapped_type::value_type value;
};

template<typename M>
backtrackable_map_at_ra_erase<M>::backtrackable_map_at_ra_erase(const M::key_type& key, const M::mapped_type::value_type& value) :
    key(key),
    value(value) {
}

template<typename M>
void backtrackable_map_at_ra_erase<M>::invoke() {
    this->ref().at(key).erase(value);
}

template<typename M>
void backtrackable_map_at_ra_erase<M>::backtrack() {
    this->ref().at(key).insert(value);
}

#endif
