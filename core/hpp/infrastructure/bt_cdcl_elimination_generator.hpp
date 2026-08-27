#ifndef BT_CDCL_ELIMINATION_GENERATOR_HPP
#define BT_CDCL_ELIMINATION_GENERATOR_HPP

#include <cstddef>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/bt_cdcl_factor.hpp"
#include "value_objects/bt_cdcl_pair_key.hpp"
#include "value_objects/bt_cdcl_pair_key_hash.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/lineage.hpp"

struct bt_cdcl_elimination_generator {
    bt_cdcl_elimination_generator();
    std::optional<const resolution_lineage*> learn(const lemma&);
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
    void cleanup();
private:
    bt_cdcl_factor* intern_leaf(const resolution_lineage*);
    bt_cdcl_factor* intern_pair(bt_cdcl_factor*, bt_cdcl_factor*);
    bt_cdcl_factor* intern_members(const std::vector<const resolution_lineage*>&, size_t, size_t);
    void visit_leaf(bt_cdcl_factor*, std::vector<const resolution_lineage*>&);
    void propagate_visit(bt_cdcl_factor*, std::vector<const resolution_lineage*>&);
    void try_fire(bt_cdcl_factor*, std::vector<const resolution_lineage*>&);
    const resolution_lineage* find_unvisited_leaf(const bt_cdcl_factor*, size_t&) const;
    size_t visited(const bt_cdcl_factor*) const;
    void refresh(bt_cdcl_factor*);

    std::deque<bt_cdcl_factor> nodes_;
    std::unordered_map<const resolution_lineage*, bt_cdcl_factor*> leaves_;
    std::unordered_map<bt_cdcl_pair_key, bt_cdcl_factor*, bt_cdcl_pair_key_hash> pairs_;
    size_t generation_;
};

#endif
