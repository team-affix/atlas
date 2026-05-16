#ifndef I_CANDIDATE_FACTORY_HPP
#define I_CANDIDATE_FACTORY_HPP

#include "../interfaces/i_factory.hpp"
#include "../value_objects/candidate.hpp"

struct i_candidate_factory : i_factory<candidate> {
    virtual ~i_candidate_factory() = default;
};

#endif
