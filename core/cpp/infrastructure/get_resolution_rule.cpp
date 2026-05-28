#include "infrastructure/get_resolution_rule.hpp"

get_resolution_rule::get_resolution_rule(locator& loc)
    : get_rule_(loc.locate<i_get_rule>()) {}

const rule* get_resolution_rule::get(const resolution_lineage* rl) const {
    return get_rule_.get(rl->idx);
}
