#ifndef WEIGHT_STORE_HPP
#define WEIGHT_STORE_HPP

#include "frontier.hpp"
#include "rule.hpp"
#include "weight_expander.hpp"

struct weight_store : frontier<double, weight_expander> {
    weight_store();
    double total() const;
    weight_expander make_expander(const double&, const rule&) override;
#ifndef DEBUG
private:
#endif
    double cgw;
};

#endif
