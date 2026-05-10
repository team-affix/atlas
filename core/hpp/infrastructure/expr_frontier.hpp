#ifndef EXPR_FRONTIER_HPP
#define EXPR_FRONTIER_HPP

#include <unordered_map>
#include "../domain/interfaces/i_expr_frontier.hpp"

struct expr_frontier : i_expr_frontier {
    void insert(const goal_lineage*, const expr*) override;
    bool contains(const goal_lineage*) const override;
    const expr*& at(const goal_lineage*) override;
    const expr* const& at(const goal_lineage*) const override;
    void erase(const goal_lineage*) override;
    void clear() override;
private:
    std::unordered_map<const goal_lineage*, const expr*> goal_exprs;
};

#endif
