#ifndef GET_RESOLUTION_RULE_HPP
#define GET_RESOLUTION_RULE_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<typename IDb>
struct get_resolution_rule {
    explicit get_resolution_rule(const IDb& db);
    const rule* get(const resolution_lineage*) const;
private:
    const IDb& get_rule_;
};

template<typename IDb>
get_resolution_rule<IDb>::get_resolution_rule(const IDb& db) : get_rule_(db) {}

template<typename IDb>
const rule* get_resolution_rule<IDb>::get(const resolution_lineage* rl) const {
    return get_rule_.get(rl->idx);
}

#endif
