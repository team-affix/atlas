#ifndef LOCATOR_HPP
#define LOCATOR_HPP

#include "bindings.hpp"

struct locator {
    static void register_bindings(const bindings* b);
    template<typename T>
    static T& resolve();
private:
    static const bindings* b;
};

template<typename T>
T& locator::resolve() {
    return b->resolve<T>();
}

#endif
