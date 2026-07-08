#ifndef BACKTRACKABLE_ASSIGN_HPP
#define BACKTRACKABLE_ASSIGN_HPP

#include <utility>
#include "infrastructure/backtrackable_mutation.hpp"

template<typename T>
struct backtrackable_assign : backtrackable_mutation<T> {
    backtrackable_assign(T new_value);
    void invoke() override;
    void backtrack() override;
private:
    T value;
};

template<typename T>
backtrackable_assign<T>::backtrackable_assign(T new_value) : value(std::move(new_value)) {
}

template<typename T>
void backtrackable_assign<T>::invoke() {
    std::swap(this->ref(), value);
}

template<typename T>
void backtrackable_assign<T>::backtrack() {
    std::swap(this->ref(), value);
}

#endif
