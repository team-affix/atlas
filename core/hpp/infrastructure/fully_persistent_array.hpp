#ifndef FULLY_PERSISTENT_ARRAY_HPP
#define FULLY_PERSISTENT_ARRAY_HPP

#include <map>
#include <optional>
#include <unordered_map>
#include "value_objects/om_interval.hpp"
#include "value_objects/om_label.hpp"

template<typename Key, typename Value>
struct fully_persistent_array {
    fully_persistent_array();
    void record(om_interval interval, Key key, Value value);
    std::optional<Value> query(om_label open_label, Key key) const;
private:
    using timeline_t = std::map<om_label, std::optional<Value>>;
    std::unordered_map<Key, timeline_t> timelines_;
};

template<typename Key, typename Value>
fully_persistent_array<Key, Value>::fully_persistent_array() {}

template<typename Key, typename Value>
void fully_persistent_array<Key, Value>::record(om_interval interval, Key key, Value value) {
    timeline_t& timeline = timelines_[key];

    std::optional<Value> prior_value = std::nullopt;
    auto it = timeline.lower_bound(interval.open);
    if (it != timeline.begin()) {
        --it;
        prior_value = it->second;
    }

    timeline[interval.open]  = value;
    timeline[interval.close] = prior_value;
}

template<typename Key, typename Value>
std::optional<Value> fully_persistent_array<Key, Value>::query(om_label open_label, Key key) const {
    const auto tl_it = timelines_.find(key);
    if (tl_it == timelines_.end())
        return std::nullopt;

    const timeline_t& timeline = tl_it->second;

    auto it = timeline.upper_bound(open_label);
    if (it == timeline.begin())
        return std::nullopt;
    --it;
    return it->second;
}

#endif
