#ifndef I_CANDIDATE_NOT_APPLICABLE_DETECTOR_HPP
#define I_CANDIDATE_NOT_APPLICABLE_DETECTOR_HPP

#include <cstdint>
#include "../value_objects/lineage.hpp"

struct i_candidate_not_applicable_detector {
    virtual ~i_candidate_not_applicable_detector() = default;
    virtual void add_candidate(const resolution_lineage*) = 0;
    virtual void remove_candidate(const resolution_lineage*) = 0;
    virtual void primary_rep_changed(uint32_t var_index) = 0;
    virtual void unify_continue(const resolution_lineage*) = 0;  // handles both resuming and functor_completed
    virtual void unify_finished(const resolution_lineage*) = 0;
    virtual void secondary_unify_failed(const resolution_lineage*) = 0;
};

#endif
