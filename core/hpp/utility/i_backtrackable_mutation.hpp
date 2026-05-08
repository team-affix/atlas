#ifndef I_REVERSIBLE_MUTATION_HPP
#define I_REVERSIBLE_MUTATION_HPP

#include "i_backtrackable.hpp"

template<typename T>
struct i_backtrackable_mutation : i_backtrackable {
    virtual ~i_backtrackable_mutation() = default;
    void capture(T&);
    virtual void invoke() = 0;
protected:
    T& ref() const;
private:
    T* pointer;
};

template<typename T>
void i_backtrackable_mutation<T>::capture(T& t) {
    pointer = &t;
}

template<typename T>
T& i_backtrackable_mutation<T>::ref() const {
    return *pointer;
}

#endif
