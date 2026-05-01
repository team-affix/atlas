#ifndef ACTIVE_GOAL_STORE_HPP
#define ACTIVE_GOAL_STORE_HPP

#include <unordered_set>
#include "../domain/interfaces/i_active_goal_store.hpp"

struct active_goal_store : i_active_goal_store {
    active_goal_store();
    void insert(const goal_lineage*) override;
    void erase(const goal_lineage*) override;
    void clear() override;
    bool contains(const goal_lineage*) override;
#ifndef DEBUG
private:
#endif
    std::unordered_set<const goal_lineage*> active_goals;
};

#endif