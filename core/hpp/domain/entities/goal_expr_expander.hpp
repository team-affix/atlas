#ifndef GOAL_EXPR_EXPANDER_HPP
#define GOAL_EXPR_EXPANDER_HPP

#include <memory>
#include <vector>
#include "../interfaces/i_goal_expr_expander.hpp"
#include "../interfaces/i_translation_map.hpp"
#include "../value_objects/expr.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_goal_expr_store.hpp"
#include "../interfaces/i_unifier.hpp"
#include "../interfaces/i_copier.hpp"

struct goal_expr_expander : i_goal_expr_expander {
    goal_expr_expander();
    void start_expansion(const resolution_lineage*) override;
    void expand_child(const goal_lineage*) override;
private:
    i_database& db;
    i_goal_expr_store& ges;
    i_unifier& bm;
    i_copier& cp;
    std::vector<const expr*> rule_body;
    std::unique_ptr<i_translation_map> tm;
};

#endif
