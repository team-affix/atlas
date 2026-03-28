#ifndef WEIGHT_STORE_HPP
#define WEIGHT_STORE_HPP

#include "frontier.hpp"
#include "rule.hpp"

struct weight_store : frontier<double> {
    weight_store(
        const goals&,
        const database&,
        lineage_pool&
    );
    std::vector<double> expand(const double&, const rule&) override;
};

#endif
