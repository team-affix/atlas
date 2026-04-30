#ifndef TRACKED_MAP_BASE_HPP
#define TRACKED_MAP_BASE_HPP

#include <utility>
#include <memory>
#include "../domain/interfaces/i_tracked_map_base.hpp"
#include "../domain/interfaces/i_tracked.hpp"
#include "trail.hpp"

template<typename M>
struct tracked_map_base : i_tracked_map_base<M> {
    tracked_map_base(trail&);
    std::pair<typename M::const_iterator, bool> upsert(const M::key_type&, const M::value_type&) override;
    std::pair<typename M::const_iterator, bool> erase(const M::key_type&) override;
    const M& get() const override;
#ifndef DEBUG
private:
#endif
    std::unique_ptr<i_tracked<M>> members;
};

template<typename M>
std::pair<typename M::const_iterator, bool> tracked_map_base<M>::upsert(const M::key_type& key, const M::value_type& value) {
    auto it = members->get().find(key);
    bool inserted = it == members->get().end();
    if (inserted) {
        members->mutate([key, value](M& m) { m.insert({key, value}); }, [key](M& m) { m.erase(key); });
    }
    else {
        members->mutate([key, value](M& m) { m.at(key) = value; }, [key, old = it->second](M& m) { m.at(key) = old; });
    }
    return {it, inserted};
}

template<typename M>
std::pair<typename M::const_iterator, bool> tracked_map_base<M>::erase(const M::key_type& key) {
    auto it = members.find(key);
    if (it == members.end())
        return {it, false};
    members->mutate([key](M& m) { m.erase(key); }, [key, old=it->second](M& m) { m.insert({key, old}); });
    return {it, true};
}

template<typename M>
const M& tracked_map_base<M>::get() const {
    return members;
}

#endif
