#ifndef TRACKED_HPP
#define TRACKED_HPP

#include <memory>
#include "infrastructure/backtrackable_mutation.hpp"
#include "interfaces/i_backtrackable.hpp"

template<typename T, typename ILog>
struct tracked {
    tracked(ILog& t, const T& initial);
    void mutate(std::unique_ptr<backtrackable_mutation<T>>);
    const T& get() const;
private:
    ILog& t;
    T value;
};

template<typename T, typename ILog>
tracked<T, ILog>::tracked(ILog& t, const T& initial) : t(t), value(initial) {}

template<typename T, typename ILog>
void tracked<T, ILog>::mutate(std::unique_ptr<backtrackable_mutation<T>> m) {
    m->capture(value);
    m->invoke();
    t.log(std::move(m));
}

template<typename T, typename ILog>
const T& tracked<T, ILog>::get() const {
    return value;
}

#endif
