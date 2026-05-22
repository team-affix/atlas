#ifndef MHU_ELIMINATION_GENERATOR_HPP
#define MHU_ELIMINATION_GENERATOR_HPP

#include <unordered_map>
#include <unordered_set>
#include "../interfaces/i_mhu_elimination_generator.hpp"
#include "../interfaces/i_bind_map.hpp"
#include "../interfaces/i_expr_pool.hpp"
#include "../utility/state_machine.hpp"
#include "../value_objects/unify_head.hpp"

struct mhu_elimination_generator : i_mhu_elimination_generator {
    virtual ~mhu_elimination_generator() = default;
    mhu_elimination_generator(
        i_bind_map&,
        i_expr_pool&);
    void add_head(const resolution_lineage*, unify_head, const std::unordered_set<uint32_t>&) override;
    void try_remove_head(const resolution_lineage*) override;
    state_machine<const resolution_lineage*> constrain(const resolution_lineage*) override;
private:
    state_machine<const resolution_lineage*> revalidate(uint32_t, const expr*);
    bool unify_and_link(const resolution_lineage*, const expr*, const expr*);
    void link(const std::unordered_set<uint32_t>&, const std::unordered_set<const resolution_lineage*>&);
    std::unordered_set<const resolution_lineage*> unlink(uint32_t);
    std::unordered_set<uint32_t> unlink(const resolution_lineage*);
    
    i_bind_map& common_;
    i_expr_pool& expr_pool_;
    
    std::unordered_map<const resolution_lineage*, unify_head> heads_;
    std::unordered_map<uint32_t, std::unordered_set<const resolution_lineage*>> rep_to_rls_;
    std::unordered_map<const resolution_lineage*, std::unordered_set<uint32_t>> rl_to_reps_;
};

#endif
