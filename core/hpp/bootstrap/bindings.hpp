#ifndef BINDINGS_HPP
#define BINDINGS_HPP

#include <unordered_map>
#include <typeindex>
#include "debug_assert.hpp"

struct bindings {
    template<typename T>
    void bind(T& value);
    template<typename T>
    T& resolve() const;
private:
    std::unordered_map<std::type_index, void*> entries;
};

template<typename T>
void bindings::bind(T& value) {
    std::type_index type{typeid(T)};
    auto [_, success] = entries.insert({type, &value});
    DEBUG_ASSERT(success);
}

template<typename T>
T& bindings::resolve() const {
    std::type_index type{typeid(T)};
    return *reinterpret_cast<T*>(entries.at(type));
}

#endif
