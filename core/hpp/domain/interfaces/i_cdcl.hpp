#ifndef I_CDCL_HPP
#define I_CDCL_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/lemma.hpp"
#include "../value_objects/avoidance.hpp"

struct i_cdcl {
    virtual ~i_cdcl() = default;
    virtual void learn(const lemma&) = 0;
    virtual void constrain(const resolution_lineage*) = 0;
    virtual void produce_events() = 0;
    virtual const avoidance& get_avoidance(size_t) = 0;
};

#endif
