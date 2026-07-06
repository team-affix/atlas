#ifndef BACKTRACKABLE_VECTOR_POP_BACK_HPP
#define BACKTRACKABLE_VECTOR_POP_BACK_HPP

#include <utility>
#include "debug_assert.hpp"
#include "infrastructure/backtrackable_mutation.hpp"

template<typename V>
struct backtrackable_vector_pop_back : backtrackable_mutation<V> {
    void invoke() override;
    void backtrack() override;
private:
    V::value_type value;
};

template<typename V>
void backtrackable_vector_pop_back<V>::invoke() {
    DEBUG_ASSERT(!this->ref().empty());
    value = std::move(this->ref().back());
    this->ref().pop_back();
}

template<typename V>
void backtrackable_vector_pop_back<V>::backtrack() {
    this->ref().push_back(std::move(value));
}

#endif
