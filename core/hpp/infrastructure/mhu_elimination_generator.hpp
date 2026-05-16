#ifndef MHU_ELIMINATION_GENERATOR_HPP
#define MHU_ELIMINATION_GENERATOR_HPP

#include <unordered_map>
#include <unordered_set>
#include "../interfaces/i_mhu_elimination_generator.hpp"
#include "../interfaces/i_unifier.hpp"
#include "../interfaces/i_factory.hpp"
#include "../interfaces/i_bind_map.hpp"
#include "../interfaces/i_overlay_bind_map.hpp"
#include "../interfaces/i_copier.hpp"
#include "../interfaces/i_expr_pool.hpp"
#include "../value_objects/unify_head.hpp"
#include "../utility/state_machine.hpp"

struct mhu_elimination_generator : i_mhu_elimination_generator {
    virtual ~mhu_elimination_generator() = default;
    mhu_elimination_generator(
        i_factory<i_unifier, i_bind_map&>&,
        i_factory<i_bind_map>&,
        i_factory<i_overlay_bind_map, i_bind_map&, i_bind_map&>&,
        i_bind_map&,
        i_copier&,
        i_expr_pool&);
    bool add_head(const resolution_lineage*, const expr*, const expr*) override;
    void remove_head(const resolution_lineage*) override;
    state_machine<const resolution_lineage*> constrain(const resolution_lineage*) override;
private:
    state_machine<const resolution_lineage*> revalidate(uint32_t, const expr*);
    bool unify_and_link(const resolution_lineage*, const expr*, const expr*);
    void link(const std::unordered_set<uint32_t>&, const std::unordered_set<const resolution_lineage*>&);
    std::unordered_set<const resolution_lineage*> unlink(uint32_t);
    std::unordered_set<uint32_t> unlink(const resolution_lineage*);
    void extract_child_reps(const expr*, std::unordered_set<uint32_t>&);
    
    i_factory<i_unifier, i_bind_map&>& unifier_factory_;
    i_factory<i_bind_map>& bind_map_factory_;
    i_factory<i_overlay_bind_map, i_bind_map&, i_bind_map&>& overlay_bind_map_factory_;
    i_bind_map& common_;
    i_copier& copier_;
    i_expr_pool& expr_pool_;
    
    std::unordered_map<const resolution_lineage*, unify_head> heads_;
    std::unordered_map<uint32_t, std::unordered_set<const resolution_lineage*>> rep_to_rls_;
    std::unordered_map<const resolution_lineage*, std::unordered_set<uint32_t>> rl_to_reps_;
};

#endif
