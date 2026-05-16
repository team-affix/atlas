#ifndef BACKTRACKABLE_SET_EXTRACT_HPP
#define BACKTRACKABLE_SET_EXTRACT_HPP

#include <utility>
#include "debug_assert.hpp"
#include "i_backtrackable_mutation.hpp"

template<typename S>
struct backtrackable_set_extract : i_backtrackable_mutation<S> {
    explicit backtrackable_set_extract(const typename S::value_type& key);
    void invoke() override;
    void backtrack() override;
    typename S::node_type& extracted();
private:
    typename S::value_type key_;
    typename S::node_type node_;
};

template<typename S>
backtrackable_set_extract<S>::backtrackable_set_extract(const typename S::value_type& key) : key_(key) {
}

template<typename S>
void backtrackable_set_extract<S>::invoke() {
    node_ = this->ref().extract(this->ref().find(key_));
    DEBUG_ASSERT(!node_.empty());
}

template<typename S>
typename S::value_type& backtrackable_set_extract<S>::extracted() {
    return node_;
}

template<typename S>
void backtrackable_set_extract<S>::backtrack() {
    this->ref().insert(std::move(node_));
}

#endif
