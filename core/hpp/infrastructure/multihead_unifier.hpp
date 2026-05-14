#ifndef MULTIHEAD_UNIFIER_HPP
#define MULTIHEAD_UNIFIER_HPP

#include <unordered_map>
#include <unordered_set>
#include "../domain/interfaces/i_multihead_unifier.hpp"
#include "../domain/interfaces/i_database.hpp"
#include "../domain/interfaces/i_frontier.hpp"
#include "../domain/interfaces/i_unifier.hpp"
#include "../domain/interfaces/i_factory.hpp"
#include "../domain/interfaces/i_bind_map.hpp"
#include "../domain/interfaces/i_overlay_bind_map.hpp"
#include "../domain/interfaces/i_translation_map.hpp"
#include "../domain/interfaces/i_copier.hpp"
#include "../domain/interfaces/i_rep_change_sink.hpp"
#include "../domain/value_objects/unify_head.hpp"
#include <memory>

struct multihead_unifier : i_multihead_unifier {
    virtual ~multihead_unifier() = default;
    multihead_unifier();
    void add_head(const resolution_lineage*) override;
    void remove_head(const resolution_lineage*) override;
    void accept(const resolution_lineage*) override;
    void reroot(uint32_t) override;
private:
    void link(const std::unordered_set<uint32_t>&, const std::unordered_set<const resolution_lineage*>&);
    std::unordered_set<const resolution_lineage*> unlink(uint32_t);
    std::unordered_set<uint32_t> unlink(const resolution_lineage*);
    void extract_reps(uint32_t, std::unordered_set<uint32_t>&);
    const i_database& db_;
    const i_frontier& frontier_;
    i_factory<i_unifier, std::unique_ptr<i_bind_map>>& unifier_factory_;
    i_factory<i_overlay_bind_map, i_bind_map&>& overlay_bind_map_factory_;
    i_bind_map& common_;
    i_factory<i_translation_map>& translation_map_factory_;
    i_copier& copier_;
    i_factory<i_rep_change_sink>& rep_change_sink_factory_;
    std::unordered_map<const resolution_lineage*, unify_head> heads_;
    std::unordered_map<uint32_t, std::unordered_set<const resolution_lineage*>> rep_to_rls_;
    std::unordered_map<const resolution_lineage*, std::unordered_set<uint32_t>> rl_to_reps_;
};

#endif
