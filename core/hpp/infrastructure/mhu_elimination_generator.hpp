#ifndef MHU_ELIMINATION_GENERATOR_HPP
#include "infrastructure/locator.hpp"

#define MHU_ELIMINATION_GENERATOR_HPP

#include <unordered_map>
#include <unordered_set>
#include "interfaces/i_elimination_generator.hpp"
#include "interfaces/i_bind_map.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_make_var.hpp"
#include "interfaces/i_bind_map_factory.hpp"
#include "interfaces/i_overlay_bind_map_factory.hpp"
#include "interfaces/i_unifier_factory.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_try_add_mhu_head.hpp"
#include "interfaces/i_clear_mhu_heads.hpp"
#include "infrastructure/state_machine.hpp"
#include "value_objects/unify_head.hpp"

struct mhu_elimination_generator
    : i_elimination_generator
    , i_try_add_mhu_head
    , i_clear_mhu_heads {
    virtual ~mhu_elimination_generator() = default;
    mhu_elimination_generator(locator& loc);
    bool try_add_head(const resolution_lineage*, const expr*, const expr*) override;
    state_machine<const resolution_lineage*> constrain(const resolution_lineage*) override;
    void clear_mhu_heads() override;
private:
    state_machine<const resolution_lineage*> rebase(uint32_t, const expr*);
    bool unify_and_link(i_unifier&, const resolution_lineage*, const expr*, const expr*);
    void link(const std::unordered_set<uint32_t>&, const std::unordered_set<const resolution_lineage*>&);
    std::unordered_set<const resolution_lineage*> unlink(uint32_t);
    std::unordered_set<uint32_t> unlink(const resolution_lineage*);
    void remove_head(const resolution_lineage*);
    
    i_bind_map& common_;
    i_make_resolution_lineage& make_resolution_lineage_;
    i_make_var& make_var_;
    i_bind_map_factory& bind_map_factory_;
    i_overlay_bind_map_factory& overlay_bind_map_factory_;
    i_unifier_factory& unifier_factory_;
    const i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids_;
    
    std::unordered_map<const resolution_lineage*, unify_head> heads_;
    std::unordered_map<uint32_t, std::unordered_set<const resolution_lineage*>> rep_to_rls_;
    std::unordered_map<const resolution_lineage*, std::unordered_set<uint32_t>> rl_to_reps_;
};

#endif
