#ifndef I_CDCL_HPP
#define I_CDCL_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/lemma.hpp"

struct i_cdcl {
    using avoidance_type = std::unordered_set<const resolution_lineage*>;
    virtual ~i_cdcl() = default;
    virtual void learn(const lemma&) = 0;
    virtual void init_constrain(const resolution_lineage*) = 0;
    virtual void resume_constrain() = 0;
    virtual bool contains(const avoidance_type&) = 0;
};

#endif
