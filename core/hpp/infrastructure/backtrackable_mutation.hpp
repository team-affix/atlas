#ifndef BACKTRACKABLE_MUTATION_HPP
#define BACKTRACKABLE_MUTATION_HPP

#include "interfaces/i_backtrackable.hpp"

template<typename T>
struct backtrackable_mutation : i_backtrackable {
    virtual ~backtrackable_mutation() = default;
    void capture(T&);
    virtual void invoke() = 0;
protected:
    T& ref() const;
private:
    T* pointer;
};

template<typename T>
void backtrackable_mutation<T>::capture(T& t) {
    pointer = &t;
}

template<typename T>
T& backtrackable_mutation<T>::ref() const {
    return *pointer;
}

#endif
