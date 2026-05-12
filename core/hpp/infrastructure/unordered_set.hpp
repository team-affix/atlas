#ifndef UNORDERED_SET_HPP
#define UNORDERED_SET_HPP

#include <unordered_set>
#include "../domain/interfaces/i_set.hpp"

template<typename T>
struct unordered_set : i_set<T> {
    void insert(T v) override;
    bool contains(T v) const override;
    void erase(T v) override;
    void clear() override;
    bool empty() const override;
    size_t size() const override;
private:
    std::unordered_set<T> s_;
};

#endif
