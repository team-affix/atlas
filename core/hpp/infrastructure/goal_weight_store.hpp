#ifndef GOAL_WEIGHT_STORE_HPP
#define GOAL_WEIGHT_STORE_HPP

#include <unordered_map>
#include "../domain/interfaces/i_goal_weight_store.hpp"

struct goal_weight_store : i_goal_weight_store {
    void insert(const goal_lineage*, double) override;
    void erase(const goal_lineage*) override;
    void clear() override;
    bool contains(const goal_lineage*) const override;
    double& at(const goal_lineage*) override;
    const double& at(const goal_lineage*) const override;
private:
    std::unordered_map<const goal_lineage*, double> goal_weights;
};

#endif
