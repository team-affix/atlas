#ifndef RESOLUTION_MEMORY_HPP
#define RESOLUTION_MEMORY_HPP

#include <unordered_set>
#include "interfaces/i_clear_recorded_resolutions.hpp"
#include "interfaces/i_derive_resolution_lemma.hpp"
#include "interfaces/i_get_resolution_count.hpp"
#include "interfaces/i_record_resolution.hpp"

struct resolution_memory
    : i_record_resolution
    , i_clear_recorded_resolutions
    , i_get_resolution_count
    , i_derive_resolution_lemma {
    void record_resolution(const resolution_lineage*) override;
    void clear_recorded_resolutions() override;
    size_t get_resolution_count() const override;
    lemma derive_resolution_lemma() const override;
private:
    std::unordered_set<const resolution_lineage*> resolutions;
};

#endif
