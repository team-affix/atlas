#ifndef PUD_TRAIL_CACHE_HPP
#define PUD_TRAIL_CACHE_HPP

#include <cstddef>
#include <list>
#include <unordered_map>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_global_node_id_hash.hpp"
#include "value_objects/pud_node_trails.hpp"

// The trails visits left, per node, kept while they fit. Trails are only ever a shortcut: a node without any is
// queried afresh, with the same result. They are dropped, least recently used first, only when trimmed.
struct pud_trail_cache {
    pud_trail_cache(size_t budget_bytes);
    const pud_node_trails* find(pud_global_node_id node);
    void store(pud_global_node_id node, pud_node_trails trails);
    void erase(pud_global_node_id node);
    void trim();
private:
    using recency_t = std::list<pud_global_node_id>;
    struct entry {
        pud_node_trails trails;
        size_t bytes;
        recency_t::iterator recency;
    };

    size_t budget_bytes_;
    size_t kept_bytes_;
    recency_t recency_;
    std::unordered_map<pud_global_node_id, entry, pud_global_node_id_hash> entries_;
};

#endif
