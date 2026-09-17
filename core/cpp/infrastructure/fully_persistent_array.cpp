#include "infrastructure/fully_persistent_array.hpp"

fully_persistent_array::fully_persistent_array() {}

void fully_persistent_array::record(om_interval interval,
                                    uint32_t var_id, framed_expr value) {
    timeline_t& timeline = timelines_[var_id];

    std::optional<framed_expr> prior_value = std::nullopt;
    auto it = timeline.lower_bound(interval.open);
    if (it != timeline.begin()) {
        --it;
        prior_value = it->second;
    }

    timeline[interval.open]  = value;
    timeline[interval.close] = prior_value;
}

std::optional<framed_expr> fully_persistent_array::query(om_label open_label,
                                                          uint32_t var_id) const {
    const auto tl_it = timelines_.find(var_id);
    if (tl_it == timelines_.end())
        return std::nullopt;

    const timeline_t& timeline = tl_it->second;

    auto it = timeline.upper_bound(open_label);
    if (it == timeline.begin())
        return std::nullopt;
    --it;
    return it->second;
}
