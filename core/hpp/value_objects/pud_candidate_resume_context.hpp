#ifndef PUD_CANDIDATE_RESUME_CONTEXT_HPP
#define PUD_CANDIDATE_RESUME_CONTEXT_HPP

#include "value_objects/pud_candidate_justification.hpp"

template<typename Descent>
struct pud_candidate_resume_context {
    pud_candidate_justification justification;
    Descent descent;
};

#endif
