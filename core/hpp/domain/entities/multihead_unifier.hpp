#ifndef MULTIHEAD_UNIFIER_HPP
#define MULTIHEAD_UNIFIER_HPP

#include <unordered_map>
#include <unordered_set>
#include "../interfaces/i_multihead_unifier.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_frontier.hpp"
#include "../interfaces/i_unifier.hpp"
#include "../interfaces/i_factory.hpp"
#include "../interfaces/i_bind_map.hpp"
#include "../interfaces/i_overlay_bind_map.hpp"
#include "../interfaces/i_copier.hpp"
#include "../interfaces/i_expr_pool.hpp"
#include "../events/head_unify_failed_event.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../value_objects/unify_head.hpp"

struct multihead_unifier : i_multihead_unifier {
    virtual ~multihead_unifier() = default;
    multihead_unifier();
    void add_head(const resolution_lineage*) override;
    void remove_head(const resolution_lineage*) override;
    void accept_head(const resolution_lineage*) override;
private:
    void revalidate(uint32_t, const expr*);
    void link(const std::unordered_set<uint32_t>&, const std::unordered_set<const resolution_lineage*>&);
    std::unordered_set<const resolution_lineage*> unlink(uint32_t);
    std::unordered_set<uint32_t> unlink(const resolution_lineage*);
    void extract_child_reps(const expr*, std::unordered_set<uint32_t>&);
    const i_database& db_;
    const i_frontier& frontier_;
    i_factory<i_unifier, i_bind_map&>& unifier_factory_;
    i_factory<i_bind_map>& bind_map_factory_;
    i_factory<i_overlay_bind_map, i_bind_map&, i_bind_map&>& overlay_bind_map_factory_;
    i_bind_map& common_;
    i_copier& copier_;
    i_expr_pool& expr_pool_;
    i_event_producer<head_unify_failed_event>& head_unify_failed_producer_;
    std::unordered_map<const resolution_lineage*, unify_head> heads_;
    std::unordered_map<uint32_t, std::unordered_set<const resolution_lineage*>> rep_to_rls_;
    std::unordered_map<const resolution_lineage*, std::unordered_set<uint32_t>> rl_to_reps_;
};

#endif
