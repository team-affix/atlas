#ifndef RESOLUTION_MEMORY_HPP
#define RESOLUTION_MEMORY_HPP

#include <unordered_set>
#include "../interfaces/i_resolution_memory.hpp"

struct resolution_memory : i_resolution_memory {
    void record_resolution(const resolution_lineage*) override;
    void clear_resolution_record() override;
    size_t get_resolution_count() const override;
    lemma derive_resolution_lemma() const override;
private:
    std::unordered_set<const resolution_lineage*> resolutions;
};

#endif
