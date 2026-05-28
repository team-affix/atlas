#ifndef LOCATOR_HPP
#define LOCATOR_HPP

#include <typeindex>
#include <unordered_map>
#include "debug_assert.hpp"

struct locator {
    template<typename... Ifaces, typename Concrete>
    void bind_as(Concrete& concrete) {
        insert(concrete);
        (insert(static_cast<Ifaces&>(concrete)), ...);
    }

    template<typename T>
    T& locate() const {
        const auto it = entries_.find(std::type_index{typeid(T)});
        DEBUG_ASSERT(it != entries_.end());
        return *static_cast<T*>(it->second);
    }

private:
    template<typename T>
    void insert(T& instance) {
        const std::type_index key{typeid(T)};
        const auto [_, inserted] = entries_.insert(
            {key, static_cast<void*>(std::addressof(instance))});
        DEBUG_ASSERT(inserted);
    }

    std::unordered_map<std::type_index, void*> entries_;
};

#endif
