#ifndef LOCATOR_HPP
#define LOCATOR_HPP

#include <typeindex>
#include <unordered_map>
#include "debug_assert.hpp"

struct locator {
    template<typename T>
    void bind(T& instance) {
        const std::type_index key{typeid(T)};
        const auto [_, inserted] = entries_.insert(
            {key, static_cast<void*>(std::addressof(instance))});
        DEBUG_ASSERT(inserted);
    }

    template<typename T>
    T& locate() const {
        const auto it = entries_.find(std::type_index{typeid(T)});
        DEBUG_ASSERT(it != entries_.end());
        return *static_cast<T*>(it->second);
    }

    template<typename T>
    bool contains() const {
        return entries_.contains(std::type_index{typeid(T)});
    }

private:
    std::unordered_map<std::type_index, void*> entries_;
};

#endif
