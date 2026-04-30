#ifndef TRACKED_SET_HPP
#define TRACKED_SET_HPP

#include <utility>
#include "trail.hpp"

template<typename S>
struct tracked_set {
    tracked_set(trail&);
    std::pair<typename S::const_iterator, bool> insert(const S::value_type&);
    std::pair<typename S::const_iterator, bool> erase(const S::value_type&);
    const S& get() const;
#ifndef DEBUG
private:
#endif
    trail& t;
    S members;
};

template<typename S>
std::pair<typename S::const_iterator, bool> tracked_set<S>::insert(const S::value_type& val) {
    auto result = members.insert(val);
    if (result.second) t.log([this, val]() { members.erase(val); });
    return result;
}

template<typename S>
std::pair<typename S::const_iterator, bool> tracked_set<S>::erase(const S::value_type& val) {
    auto result = members.erase(val);
    if (result.second) t.log([this, val]() { members.insert(val); });
    return result;
}

#endif
