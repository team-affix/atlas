#ifndef BACKTRACKABLE_SET_ERASE_HPP
#define BACKTRACKABLE_SET_ERASE_HPP

#include "debug_assert.hpp"
#include "i_backtrackable_mutation.hpp"

template<typename S>
struct backtrackable_set_erase : i_backtrackable_mutation<S> {
    backtrackable_set_erase(const S::value_type& value);
    void invoke() override;
    void backtrack() override;
private:
    S::value_type value;
};

template<typename S>
backtrackable_set_erase<S>::backtrackable_set_erase(const S::value_type& value) : value(value) {
}

template<typename S>
void backtrackable_set_erase<S>::invoke() {
    auto erased = this->ref().erase(value);
    DEBUG_ASSERT(erased == 1);
}

template<typename S>
void backtrackable_set_erase<S>::backtrack() {
    this->ref().insert(value);
}

#endif
