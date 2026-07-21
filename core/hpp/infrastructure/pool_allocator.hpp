#ifndef POOL_ALLOCATOR_HPP
#define POOL_ALLOCATOR_HPP

#include <deque>
#include <utility>
#include <vector>
#include "debug_assert.hpp"

template<typename T>
struct pool_allocator {
    pool_allocator();
    T* acquire();
    void release(T* p);
    T* emplace(T&& obj);
private:
    std::deque<T> storage_;
    std::vector<T*> free_;
};

template<typename T>
pool_allocator<T>::pool_allocator() : storage_(), free_() {}

template<typename T>
T* pool_allocator<T>::acquire() {
    if (free_.empty())
        return nullptr;
    T* p = free_.back();
    free_.pop_back();
    return p;
}

template<typename T>
void pool_allocator<T>::release(T* p) {
    DEBUG_ASSERT(p != nullptr);
    free_.push_back(p);
}

template<typename T>
T* pool_allocator<T>::emplace(T&& obj) {
    storage_.emplace_back(std::move(obj));
    return &storage_.back();
}

#endif
