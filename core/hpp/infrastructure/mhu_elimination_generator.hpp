#ifndef MHU_ELIMINATION_GENERATOR_HPP
#define MHU_ELIMINATION_GENERATOR_HPP

#include <unordered_map>
#include <unordered_set>
#include <queue>
#include "infrastructure/locator.hpp"
#include "interfaces/i_elimination_generator.hpp"
#include "interfaces/i_bind_map.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_make_var.hpp"
#include "interfaces/i_bind_map_factory.hpp"
#include "interfaces/i_unifier_factory.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_try_add_mhu_head.hpp"
#include "interfaces/i_clear_mhu_heads.hpp"
#include "infrastructure/coroutine.hpp"
#include "value_objects/unify_head.hpp"

struct mhu_elimination_generator
    : i_elimination_generator
    , i_try_add_mhu_head
    , i_clear_mhu_heads {
    virtual ~mhu_elimination_generator() = default;
    mhu_elimination_generator(locator& loc);
    bool try_add_head(const resolution_lineage*, const expr*, const expr*) override;
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*) override;
    void clear_mhu_heads() override;
private:
    coroutine<const resolution_lineage*, void> accept_bindings(i_bind_map&, const std::unordered_set<uint32_t>&);
    coroutine<const resolution_lineage*, void> rebase_all(uint32_t);
    bool sync_and_link(const resolution_lineage*, i_unifier&, std::queue<uint32_t>&);
    coroutine<uint32_t, bool> synchronize(i_unifier&, std::queue<uint32_t>&);
    void link(const std::unordered_set<uint32_t>&, const std::unordered_set<const resolution_lineage*>&);
    std::unordered_set<const resolution_lineage*> unlink(uint32_t);
    std::unordered_set<uint32_t> unlink(const resolution_lineage*);
    void remove_head(const resolution_lineage*);
    
    i_bind_map& common_;
    i_make_resolution_lineage& make_resolution_lineage_;
    i_make_var& make_var_;
    i_bind_map_factory& bind_map_factory_;
    i_unifier_factory& unifier_factory_;
    const i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids_;
    
    std::unordered_map<const resolution_lineage*, unify_head> heads_;
    std::unordered_map<uint32_t, std::unordered_set<const resolution_lineage*>> rep_to_rls_;
    std::unordered_map<const resolution_lineage*, std::unordered_set<uint32_t>> rl_to_reps_;
};

#endif
