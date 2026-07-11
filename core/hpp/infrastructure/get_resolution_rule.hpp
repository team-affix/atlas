#ifndef GET_RESOLUTION_RULE_HPP
#define GET_RESOLUTION_RULE_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<typename IGetRule>
struct get_resolution_rule {
    get_resolution_rule(const IGetRule& db);
    const rule* get(const resolution_lineage*) const;
private:
    const IGetRule& get_rule_;
};

template<typename IGetRule>
get_resolution_rule<IGetRule>::get_resolution_rule(const IGetRule& db) : get_rule_(db) {}

template<typename IGetRule>
const rule* get_resolution_rule<IGetRule>::get(const resolution_lineage* rl) const {
    return get_rule_.get_rule(rl->idx);
}

#endif
