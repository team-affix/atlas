#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include "bindings.hpp"

struct resolver {
    static void register_bindings(const bindings* b);
    template<typename T>
    static T& resolve();
#ifndef DEBUG
private:
#endif
    static const bindings* b;
};

template<typename T>
T& resolver::resolve() {
    return b->resolve<T>();
}

#endif
