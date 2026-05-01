#ifndef RESTORABLE_MAP_BASE_HPP
#define RESTORABLE_MAP_BASE_HPP

#include "../domain/interfaces/i_restorable_map.hpp"

// NOTES
// - when an entry gets added, we mark the key
// - when an entry gets subtracted,
//       we restore it on the spot and unmark as dirty.
// - when an entry gets dirty, we mark the key

template<typename M, typename S>
struct restorable_map_base : i_restorable_map<typename M::key_type, typename M::value_type> {
    void restore() override;
    bool insert(const typename M::key_type&, const typename M::value_type&) override;
    bool erase(const typename M::key_type&) override;
    typename M::value_type& at(const typename M::key_type&) override;
    const typename M::value_type& at(const typename M::key_type&) const override;
    size_t size() const override;
    bool empty() const override;
    void clear() override;
#ifndef DEBUG
private:
#endif
    M current;
    S added_keys;
    S dirty_keys;
    M subtracted_map;
};

template<typename M, typename S>
void restorable_map_base<M, S>::restore() {
    // erase all added keys
    for (const auto& key : added_keys)
        current.erase(key);

    // insert all subtracted entries
    for (auto& entry : subtracted_map)
        current.insert(entry);

    // restore all dirty entries
    for (const auto& key : dirty_keys)
        current.at(key).restore();

    added_keys.clear();
    subtracted_keys.clear();
    dirty_keys.clear();
    original_values.clear();
}

template<typename M, typename S>
void restorable_map_base<M, S>::insert(const M::key_type& key, const M::value_type& value) {
    auto [_, inserted] = current.insert({key, value});

    if (!inserted) {
        // we are inserting over an existing key, no-op
        return;
    }
    
    auto [_, subtraction_erased] = subtracted_keys.erase(key);

    if (!subtraction_erased) {
        // key did not exist in original
        added_keys.insert(key);
        return;
    }

    // key existed in original

    if (original_values.at(key) == value) {
        // we are re-inserting the original value
        return;
    }

    // we are replacing the original value with a new value
    dirty_keys.insert(key);
}

template<typename M, typename S>
void restorable_map_base<M, S>::erase(const M::key_type& key) {
    auto [_, erased] = current.extract(key);

    if (!erased) { 
        // erasure did nothing, no-op
        return;
    }
    
    auto [_, addition_erased] = added_keys.erase(key);
    
    if (addition_erased) {
        // key was not in original
        return;
    }

    // key was in the original

    auto [_, dirty_erased] = dirty_keys.erase(key);

    if (dirty_erased) {
        // key was in the original and value is dirty. remove from dirty_keys
        dirty_keys.erase(key);
    }

    // no matter what, we know key was in original so must mark
    // it as subtracted
    subtracted_keys.insert(key);
}

// assign does not touch subtracted_pairs
template<typename M, typename S>
void restorable_map_base<M, S>::assign(const M::key_type& key, const M::value_type& value) {
    current.at(key) = value;

    if (added_keys.contains(key)) {
        // key was not in original, doesn't matter what value it has
        return;
    }

    // key was in original

    if (original_values.at(key) == value) {
        // we are assigning the original value into its place.
        // remove from dirty_keys if able
        dirty_keys.erase(key);
    }
    else {
        // we are assigning a new value to a key that was in original.
        // mark as dirty if not already
        dirty_keys.insert(key);
    }
}

template<typename M, typename S>
const M& restorable_map_base<M, S>::get() const {
    return current;
}

#endif
