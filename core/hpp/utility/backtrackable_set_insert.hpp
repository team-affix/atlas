#ifndef BACKTRACKABLE_SET_INSERT_HPP
#define BACKTRACKABLE_SET_INSERT_HPP

#include "i_backtrackable_mutation.hpp"

template<typename S>
struct backtrackable_set_insert : i_backtrackable_mutation<S> {
    backtrackable_set_insert(const S::value_type& value);
    void invoke() override;
    void backtrack() override;
private:
    S::value_type value;
};

template<typename S>
backtrackable_set_insert<S>::backtrackable_set_insert(const S::value_type& value) : value(value) {
}

template<typename S>
void backtrackable_set_insert<S>::invoke() {
    this->ref().insert(value);
}

template<typename S>
void backtrackable_set_insert<S>::backtrack() {
    this->ref().erase(value);
}

#endif
