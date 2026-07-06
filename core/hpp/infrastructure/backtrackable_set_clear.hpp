#ifndef BACKTRACKABLE_SET_CLEAR_HPP
#define BACKTRACKABLE_SET_CLEAR_HPP

#include <utility>
#include "infrastructure/backtrackable_mutation.hpp"

template<typename S>
struct backtrackable_set_clear : backtrackable_mutation<S> {
    void invoke() override;
    void backtrack() override;
private:
    S saved;
};

template<typename S>
void backtrackable_set_clear<S>::invoke() {
    saved = std::move(this->ref());
    this->ref().clear();
}

template<typename S>
void backtrackable_set_clear<S>::backtrack() {
    this->ref() = std::move(saved);
}

#endif
