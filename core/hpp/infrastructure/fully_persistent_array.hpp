#ifndef FULLY_PERSISTENT_ARRAY_HPP
#define FULLY_PERSISTENT_ARRAY_HPP

#include <map>
#include <optional>
#include <unordered_map>
#include "value_objects/om_interval.hpp"
#include "value_objects/framed_expr.hpp"

struct fully_persistent_array {
    fully_persistent_array();
    void record(om_interval interval, uint32_t var_id, framed_expr value);
    std::optional<framed_expr> query(om_label open_label, uint32_t var_id) const;
private:
    using timeline_t = std::map<om_label, std::optional<framed_expr>>;
    std::unordered_map<uint32_t, timeline_t> timelines_;
};

#endif
