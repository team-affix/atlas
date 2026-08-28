#ifndef FGT_BT_CDCL_ELIMINATION_GENERATOR_HPP
#define FGT_BT_CDCL_ELIMINATION_GENERATOR_HPP

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/bt_cdcl_factor.hpp"
#include "value_objects/bt_cdcl_pair_key.hpp"
#include "value_objects/bt_cdcl_pair_key_hash.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/lineage.hpp"

struct fgt_bt_cdcl_elimination_generator {
    fgt_bt_cdcl_elimination_generator(size_t max_clauses);
    std::optional<const resolution_lineage*> learn(const lemma&);
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
    void cleanup();
private:
    using node_iter       = std::list<bt_cdcl_factor>::iterator;
    using fire_order_iter = std::list<bt_cdcl_factor*>::iterator;

    bt_cdcl_factor* intern_leaf(const resolution_lineage*);
    bt_cdcl_factor* intern_pair(bt_cdcl_factor*, bt_cdcl_factor*);
    bt_cdcl_factor* intern_members(const std::vector<const resolution_lineage*>&, size_t, size_t);
    coroutine<const resolution_lineage*, void> visit_leaf(bt_cdcl_factor*);
    coroutine<const resolution_lineage*, void> propagate_visit(bt_cdcl_factor*);
    coroutine<const resolution_lineage*, void> try_fire(bt_cdcl_factor*);
    const resolution_lineage* find_unvisited_leaf(const bt_cdcl_factor*, size_t&) const;
    size_t visited(const bt_cdcl_factor*) const;
    void refresh(bt_cdcl_factor*);
    void trim(bt_cdcl_factor*);
    void evict_oldest();
    void trim_to_capacity();

    std::list<bt_cdcl_factor>                                nodes_;
    std::unordered_map<bt_cdcl_factor*, node_iter>           node_iters_;
    std::unordered_map<const resolution_lineage*, bt_cdcl_factor*> leaves_;
    std::unordered_map<bt_cdcl_pair_key, bt_cdcl_factor*, bt_cdcl_pair_key_hash> pairs_;
    size_t generation_;

    // front = least recently fired (evict here); back = most recently fired
    std::list<bt_cdcl_factor*>                               fire_order_;
    std::unordered_map<bt_cdcl_factor*, fire_order_iter>     fire_pos_;
    size_t                                                   capacity_;
};

#endif
