#include "infrastructure/fp_bind_map.hpp"

fp_bind_map::fp_bind_map() {}

void fp_bind_map::record(om_label open, om_label close,
                         uint32_t var_id, framed_expr value) {
    timeline_t& timeline = timelines_[var_id];

    // Find the value that was in effect just before open — this is what close
    // must restore so that nodes outside this interval are unaffected.
    std::optional<framed_expr> prior_value = std::nullopt;
    auto it = timeline.lower_bound(open);
    if (it != timeline.begin()) {
        --it;
        prior_value = it->second;
    }

    timeline[open]  = value;
    timeline[close] = prior_value;
}

std::optional<framed_expr> fp_bind_map::query(om_label open_label,
                                               uint32_t var_id) const {
    const auto tl_it = timelines_.find(var_id);
    if (tl_it == timelines_.end())
        return std::nullopt;

    const timeline_t& timeline = tl_it->second;

    // Predecessor-or-equal: largest key <= open_label.
    auto it = timeline.upper_bound(open_label);
    if (it == timeline.begin())
        return std::nullopt;
    --it;
    return it->second;
}
