#ifndef GOAL_CANDIDATES_STORE_HPP
#define GOAL_CANDIDATES_STORE_HPP

#include <unordered_map>
#include "../domain/interfaces/i_goal_candidates_store.hpp"

struct goal_candidates_store : i_goal_candidates_store {
    void insert(const goal_lineage*, candidate_set) override;
    void erase(const goal_lineage*) override;
    void clear() override;
    bool contains(const goal_lineage*) const override;
    candidate_set& at(const goal_lineage*) override;
    const candidate_set& at(const goal_lineage*) const override;
private:
    std::unordered_map<const goal_lineage*, candidate_set> goal_candidates;
};

#endif
