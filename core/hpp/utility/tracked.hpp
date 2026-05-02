#ifndef TRACKED_HPP
#define TRACKED_HPP

#include <memory>
#include "i_backtrackable_mutation.hpp"
#include "i_trail.hpp"

template<typename T>
struct tracked {
    virtual ~tracked() = default;
    tracked(i_trail& t, const T& initial);
    virtual void mutate(std::unique_ptr<i_backtrackable_mutation<T>>);
    virtual const T& get() const;
private:
    i_trail& t;
    T value;
};

template<typename T>
tracked<T>::tracked(i_trail& t, const T& initial) : t(t), value(initial) {
}

template<typename T>
void tracked<T>::mutate(std::unique_ptr<i_backtrackable_mutation<T>> m) {
    m->capture(value);
    m->invoke();
    t.log(std::move(m));
}

template<typename T>
const T& tracked<T>::get() const {
    return value;
}

#endif
