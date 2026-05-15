#ifndef I_CDCL_HPP
#define I_CDCL_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/lemma.hpp"

struct i_cdcl {
    using avoidance_type = std::unordered_set<const resolution_lineage*>;
    virtual ~i_cdcl() = default;
    virtual void learn(const lemma&) = 0;
    virtual void constrain(const resolution_lineage*) = 0;
    virtual const avoidance_type& get_avoidance(size_t) = 0;
};

#endif
