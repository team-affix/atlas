#ifndef BT_CDCL_ELIMINATION_GENERATOR_HPP
#define BT_CDCL_ELIMINATION_GENERATOR_HPP

#include <cstddef>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/lineage.hpp"

struct bt_cdcl_elimination_generator {
    bt_cdcl_elimination_generator();
    std::optional<const resolution_lineage*> learn(const lemma&);
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
    void cleanup();
private:
    struct factor {
        factor(size_t tuple_size,
               factor* left,
               factor* right,
               const resolution_lineage* leaf_rl);
        friend struct bt_cdcl_elimination_generator;
    private:
        size_t tuple_size_;
        size_t visited_;
        size_t visited_generation_;
        size_t nand_multiplicity_;
        size_t fired_generation_;
        factor* left_;
        factor* right_;
        const resolution_lineage* leaf_rl_;
        std::vector<factor*> parents_;
    };

    struct pair_key {
        factor* left;
        factor* right;
        bool operator==(const pair_key&) const;
    };

    struct pair_key_hash {
        size_t operator()(const pair_key&) const;
    };

    factor* intern_leaf(const resolution_lineage*);
    factor* intern_pair(factor*, factor*);
    factor* build(const std::vector<const resolution_lineage*>&, size_t, size_t);
    size_t live_visited(const factor*) const;
    void ensure_generation(factor*);
    void visit_leaf(factor*, std::vector<const resolution_lineage*>&);
    void after_increment(factor*, std::vector<const resolution_lineage*>&);
    void check_nand(factor*, std::vector<const resolution_lineage*>&);
    size_t unvisited_count(const factor*) const;
    const resolution_lineage* unvisited_leaf(const factor*) const;

    std::deque<factor> nodes_;
    std::unordered_map<const resolution_lineage*, factor*> leaves_;
    std::unordered_map<pair_key, factor*, pair_key_hash> pairs_;
    size_t generation_;
};

#endif
