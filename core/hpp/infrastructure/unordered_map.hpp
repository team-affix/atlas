#ifndef UNORDERED_MAP_HPP
#define UNORDERED_MAP_HPP

#include <unordered_map>
#include <utility>
#include "../domain/interfaces/i_map.hpp"

template<typename K, typename V>
struct unordered_map : i_map<K, V> {
    void insert(K k, V v) override { m_.emplace(k, std::move(v)); }
    bool contains(K k) const override { return m_.count(k) > 0; }
    V& at(K k) override { return m_.at(k); }
    const V& at(K k) const override { return m_.at(k); }
    void erase(K k) override { m_.erase(k); }
    void clear() override { m_.clear(); }
private:
    std::unordered_map<K, V> m_;
};

#endif
