#ifndef WEIGHT_FRONTIER_HPP
#define WEIGHT_FRONTIER_HPP

#include <unordered_map>
#include "../domain/interfaces/i_weight_frontier.hpp"

struct weight_frontier : i_weight_frontier {
    void insert(const goal_lineage*, double) override;
    bool contains(const goal_lineage*) const override;
    double& at(const goal_lineage*) override;
    const double& at(const goal_lineage*) const override;
    void erase(const goal_lineage*) override;
    void clear() override;
private:
    std::unordered_map<const goal_lineage*, double> goal_weights;
};

#endif
