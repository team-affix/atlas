#ifndef GET_RESOLUTION_RULE_HPP
#define GET_RESOLUTION_RULE_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_get_resolution_rule.hpp"
#include "interfaces/i_get_rule.hpp"

struct get_resolution_rule : i_get_resolution_rule {
    explicit get_resolution_rule(locator& loc);
    const rule* get(const resolution_lineage*) const override;
private:
    const i_get_rule& get_rule_;
};

#endif
