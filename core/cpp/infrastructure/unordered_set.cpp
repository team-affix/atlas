#include <cstdint>
#include "../../hpp/infrastructure/unordered_set.hpp"

template<typename T>
void unordered_set<T>::insert(T v) { s_.insert(v); }

template<typename T>
bool unordered_set<T>::contains(T v) const { return s_.count(v) > 0; }

template<typename T>
void unordered_set<T>::erase(T v) { s_.erase(v); }

template<typename T>
void unordered_set<T>::clear() { s_.clear(); }

template<typename T>
bool unordered_set<T>::empty() const { return s_.empty(); }

template<typename T>
size_t unordered_set<T>::size() const { return s_.size(); }

template struct unordered_set<uint32_t>;
