#ifndef GOAL_EXPR_STORE_HPP
#define GOAL_EXPR_STORE_HPP

#include <unordered_map>
#include "../domain/interfaces/i_goal_expr_store.hpp"

struct goal_expr_store : i_goal_expr_store {
    void insert(const goal_lineage*, const expr*) override;
    void erase(const goal_lineage*) override;
    void clear() override;
    bool contains(const goal_lineage*) const override;
    const expr*& at(const goal_lineage*) override;
    const expr* const& at(const goal_lineage*) const override;
private:
    std::unordered_map<const goal_lineage*, const expr*> goal_exprs;
};

#endif
