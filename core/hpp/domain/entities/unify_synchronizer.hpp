#ifndef UNIFY_SYNCHRONIZER_HPP
#define UNIFY_SYNCHRONIZER_HPP

#include <unordered_map>
#include <unordered_set>
#include "../interfaces/i_unify_synchronizer.hpp"
#include "../interfaces/i_unifier.hpp"
#include "../interfaces/i_applicant_frontier.hpp"
#include "../interfaces/i_expr_frontier.hpp"
#include "../interfaces/i_expr_pool.hpp"
#include "../value_objects/expr.hpp"

struct unify_synchronizer : i_unify_synchronizer {
    unify_synchronizer();

    void primary_rep_changed(uint32_t var_index) override;
    void register_candidate(const resolution_lineage*) override;
    void unregister_candidate(const resolution_lineage*) override;
    void accept(const resolution_lineage*) override;

private:
    void extract_rep_vars(const expr*, std::unordered_set<uint32_t>&) const;
    void watch(const std::unordered_set<uint32_t>& vars, const std::unordered_set<const resolution_lineage*>& rls);
    std::unordered_set<const resolution_lineage*> unwatch(uint32_t var);
    std::unordered_set<uint32_t> unwatch(const resolution_lineage*);
    void update_rep_watches(uint32_t var);

    i_unifier& u;
    i_applicant_frontier& af;
    i_expr_frontier& ges;
    i_expr_pool& ep;

    std::unordered_map<uint32_t, std::unordered_set<const resolution_lineage*>> var_to_rls;
    std::unordered_map<const resolution_lineage*, std::unordered_set<uint32_t>> rl_to_vars;
};

#endif
