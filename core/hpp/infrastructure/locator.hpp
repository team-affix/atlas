#ifndef LOCATOR_HPP
#define LOCATOR_HPP

#include <unordered_map>
#include <cassert>
#include <stack>
#include <unordered_set>
#include <typeindex>

struct locator {
    template<typename T>
    static T& locate();
    template<typename T>
    static void bind(T& value);
    static void push_frame();
    static void pop_frame();
    static void clear_frame();
#ifndef DEBUG
private:
#endif
    static std::unordered_map<std::type_index, void*> entries;
    static std::unordered_set<std::type_index> current_frame_additions;
    static std::stack<std::unordered_set<std::type_index>> past_frames;
};

template<typename T>
T& locator::locate() {
    return *reinterpret_cast<T*>(entries.at(std::type_index(typeid(T))));
}

template<typename T>
void locator::bind(T& value) {
    std::type_index type{typeid(T)};
    auto [_, success] = entries.insert({type, &value});
    assert(success);
    current_frame_additions.insert(type);
}

#endif
