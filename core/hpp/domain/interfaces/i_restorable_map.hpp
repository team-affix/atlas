#ifndef I_RESTORABLE_MAP_HPP
#define I_RESTORABLE_MAP_HPP

#include <cstddef>
#include <memory>
#include "i_dirt_tracker_ref.hpp"
#include "i_restorable.hpp"

template<typename K, typename V>
struct i_restorable_map : i_restorable {
    virtual ~i_restorable_map() = default;
    virtual void restore() = 0;
    virtual bool insert(const K&, const V&) = 0;
    virtual bool erase(const K&) = 0;
    virtual std::shared_ptr<i_dirt_tracker_ref<V>> at(const K&) = 0;
    virtual const V& at(const K&) const = 0;
    virtual size_t size() const = 0;
    virtual bool empty() const = 0;
    virtual void clear() = 0;
};

#endif
