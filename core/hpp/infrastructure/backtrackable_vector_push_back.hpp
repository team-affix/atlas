#ifndef BACKTRACKABLE_VECTOR_PUSH_BACK_HPP
#define BACKTRACKABLE_VECTOR_PUSH_BACK_HPP

#include "infrastructure/backtrackable_mutation.hpp"

template<typename V>
struct backtrackable_vector_push_back : backtrackable_mutation<V> {
    backtrackable_vector_push_back(const V::value_type& value);
    void invoke() override;
    void backtrack() override;
private:
    V::value_type value;
};

template<typename V>
backtrackable_vector_push_back<V>::backtrackable_vector_push_back(const V::value_type& value) : value(value) {
}

template<typename V>
void backtrackable_vector_push_back<V>::invoke() {
    this->ref().push_back(value);
}

template<typename V>
void backtrackable_vector_push_back<V>::backtrack() {
    this->ref().pop_back();
}

#endif
