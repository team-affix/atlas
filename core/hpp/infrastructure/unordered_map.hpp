#ifndef UNORDERED_MAP_HPP
#define UNORDERED_MAP_HPP

#include <unordered_map>
#include "../domain/interfaces/i_map.hpp"

template<typename K, typename V>
struct unordered_map : i_map<K, V> {
    void insert(K k, V v) override;
    bool contains(K k) const override;
    V& at(K k) override;
    const V& at(K k) const override;
    void erase(K k) override;
    void clear() override;
private:
    std::unordered_map<K, V> m_;
};

#endif
