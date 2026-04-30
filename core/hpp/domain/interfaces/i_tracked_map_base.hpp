#ifndef I_TRACKED_MAP_BASE_HPP
#define I_TRACKED_MAP_BASE_HPP

#include <utility>

template<typename M>
struct i_tracked_map_base {
    virtual ~i_tracked_map_base() = default;
    virtual std::pair<typename M::const_iterator, bool> upsert(const M::key_type&, const M::value_type&) = 0;
    virtual std::pair<typename M::const_iterator, bool> erase(const M::key_type&) = 0;
    virtual const M& get() const = 0;
};

#endif
