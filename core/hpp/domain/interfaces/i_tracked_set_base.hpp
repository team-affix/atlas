#ifndef I_TRACKED_SET_BASE_HPP
#define I_TRACKED_SET_BASE_HPP

#include <utility>

template<typename S>
struct i_tracked_set_base {
    virtual ~i_tracked_set_base() = default;
    virtual std::pair<typename S::const_iterator, bool> insert(const S::value_type&) = 0;
    virtual std::pair<typename S::const_iterator, bool> erase(const S::value_type&) = 0;
    virtual const S& get() const = 0;
};

#endif
