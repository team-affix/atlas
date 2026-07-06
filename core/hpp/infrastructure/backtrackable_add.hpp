#ifndef BACKTRACKABLE_ADD_HPP
#define BACKTRACKABLE_ADD_HPP

#include "infrastructure/backtrackable_mutation.hpp"

template<typename T>
struct backtrackable_add : backtrackable_mutation<T> {
    backtrackable_add(T amount);
    void invoke() override;
    void backtrack() override;
private:
    T amount;
};

template<typename T>
backtrackable_add<T>::backtrackable_add(T amount) : amount(amount) {
}

template<typename T>
void backtrackable_add<T>::invoke() {
    this->ref() += amount;
}

template<typename T>
void backtrackable_add<T>::backtrack() {
    this->ref() -= amount;
}

#endif
