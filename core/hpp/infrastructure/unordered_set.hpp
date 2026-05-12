#ifndef UNORDERED_SET_HPP
#define UNORDERED_SET_HPP

#include <unordered_set>
#include "../domain/interfaces/i_set.hpp"

template<typename T>
struct unordered_set : i_set<T> {
    void insert(T v) override { s_.insert(v); }
    bool contains(T v) const override { return s_.count(v) > 0; }
    void erase(T v) override { s_.erase(v); }
    void clear() override { s_.clear(); }
    bool empty() const override { return s_.empty(); }
    size_t size() const override { return s_.size(); }
private:
    std::unordered_set<T> s_;
};

#endif
