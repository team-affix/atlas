#ifndef BACKTRACKABLE_DEQUE_EMPLACE_BACK_HPP
#define BACKTRACKABLE_DEQUE_EMPLACE_BACK_HPP

#include <utility>
#include "infrastructure/backtrackable_mutation.hpp"

// Appends a (possibly move-only) element to the back of a deque, undoing with
// pop_back. Because a deque never relocates its existing elements on end-ops,
// addresses of previously appended elements remain stable across further pushes
// and pops, which the MHU head arena relies on for its deferred destruction.
template<typename D>
struct backtrackable_deque_emplace_back : backtrackable_mutation<D> {
    backtrackable_deque_emplace_back(D::value_type&& value);
    void invoke() override;
    void backtrack() override;
private:
    D::value_type value;
};

template<typename D>
backtrackable_deque_emplace_back<D>::backtrackable_deque_emplace_back(D::value_type&& value) : value(std::move(value)) {
}

template<typename D>
void backtrackable_deque_emplace_back<D>::invoke() {
    this->ref().emplace_back(std::move(value));
}

template<typename D>
void backtrackable_deque_emplace_back<D>::backtrack() {
    this->ref().pop_back();
}

#endif
