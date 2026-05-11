#ifndef LOCATOR_HPP
#define LOCATOR_HPP

#include "bindings.hpp"

struct locator {
    static void register_bindings(const bindings* b);
    template<typename T>
    static T& locate();
private:
    static const bindings* b;
};

template<typename T>
T& locator::locate() {
    return b->resolve<T>();
}

#endif
