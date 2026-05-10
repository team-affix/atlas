#ifndef CANDIDATES_FRONTIER_HPP
#define CANDIDATES_FRONTIER_HPP

#include <unordered_map>
#include "../domain/interfaces/i_candidates_frontier.hpp"

struct candidates_frontier : i_candidates_frontier {
    void insert(const goal_lineage*, candidate_set) override;
    bool contains(const goal_lineage*) const override;
    candidate_set& at(const goal_lineage*) override;
    const candidate_set& at(const goal_lineage*) const override;
    void erase(const goal_lineage*) override;
    void clear() override;
private:
    std::unordered_map<const goal_lineage*, candidate_set> goal_candidates;
};

#endif
