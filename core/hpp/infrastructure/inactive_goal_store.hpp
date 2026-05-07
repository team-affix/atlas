#ifndef INACTIVE_GOAL_STORE_HPP
#define INACTIVE_GOAL_STORE_HPP

#include <unordered_set>
#include "../domain/interfaces/i_inactive_goal_store.hpp"

struct inactive_goal_store : i_inactive_goal_store {
    inactive_goal_store();
    void insert(const goal_lineage*) override;
    void erase(const goal_lineage*) override;
    void clear() override;
    bool contains(const goal_lineage*) override;
private:
    std::unordered_set<const goal_lineage*> inactive_goals;
};

#endif
