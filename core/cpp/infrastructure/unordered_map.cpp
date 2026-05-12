#include <cstddef>
#include <memory>
#include "../../hpp/infrastructure/unordered_map.hpp"
#include "../../hpp/domain/interfaces/i_candidate.hpp"

template<typename K, typename V>
void unordered_map<K, V>::insert(K k, V v) { m_.emplace(k, std::move(v)); }

template<typename K, typename V>
bool unordered_map<K, V>::contains(K k) const { return m_.count(k) > 0; }

template<typename K, typename V>
V& unordered_map<K, V>::at(K k) { return m_.at(k); }

template<typename K, typename V>
const V& unordered_map<K, V>::at(K k) const { return m_.at(k); }

template<typename K, typename V>
void unordered_map<K, V>::erase(K k) { m_.erase(k); }

template<typename K, typename V>
void unordered_map<K, V>::clear() { m_.clear(); }

template struct unordered_map<size_t, std::unique_ptr<i_candidate>>;
