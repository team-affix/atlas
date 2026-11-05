#ifndef PUD_CHUNKED_REGISTER_APPLIER_HPP
#define PUD_CHUNKED_REGISTER_APPLIER_HPP

#include <algorithm>
#include <cstddef>
#include <vector>
#include "value_objects/pud_frontier_change.hpp"

// Applies a registration's frontier change a chunk of offspring at a time, trimming the caches between chunks. Taking
// a large stored tree's leaves into the frontier reads each one's state, and no search step ends meanwhile to trim
// them; in chunks they stay within their budgets. The refuted are applied with the first chunk.
template<typename IApplyRegister, typename ITrimCaches>
struct pud_chunked_register_applier {
    pud_chunked_register_applier(IApplyRegister& apply_register, ITrimCaches& trim_caches, size_t chunk_size);
    void apply_register(const pud_frontier_change& change);
private:
    IApplyRegister& apply_register_;
    ITrimCaches& trim_caches_;
    size_t chunk_size_;
};

template<typename IApplyRegister, typename ITrimCaches>
pud_chunked_register_applier<IApplyRegister, ITrimCaches>::pud_chunked_register_applier(
    IApplyRegister& apply_register, ITrimCaches& trim_caches, size_t chunk_size)
    : apply_register_(apply_register)
    , trim_caches_(trim_caches)
    , chunk_size_(chunk_size) {}

template<typename IApplyRegister, typename ITrimCaches>
void pud_chunked_register_applier<IApplyRegister, ITrimCaches>::apply_register(const pud_frontier_change& change) {
    pud_frontier_change chunk{.offspring = {}, .refuted = change.refuted};
    for (size_t start = 0; start == 0 || start < change.offspring.size(); start += chunk_size_) {
        const size_t end = std::min(start + chunk_size_, change.offspring.size());
        chunk.offspring.assign(change.offspring.begin() + start, change.offspring.begin() + end);
        apply_register_.apply_register(chunk);
        trim_caches_.trim();
        chunk.refuted.clear();
    }
}

#endif
