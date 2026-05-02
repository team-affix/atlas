#ifndef BACKTRACKABLE_INCREMENT_HPP
#define BACKTRACKABLE_INCREMENT_HPP

#include "i_backtrackable_mutation.hpp"

template<typename T>
struct backtrackable_increment : i_backtrackable_mutation<T> {
    void invoke() override;
    void backtrack() override;
};

template<typename T>
void backtrackable_increment<T>::invoke() {
    ++this->ref();
}

template<typename T>
void backtrackable_increment<T>::backtrack() {
    --this->ref();
}

#endif
