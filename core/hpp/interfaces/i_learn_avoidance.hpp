#ifndef I_LEARN_AVOIDANCE_HPP
#define I_LEARN_AVOIDANCE_HPP

#include <optional>
#include "value_objects/lemma.hpp"
#include "value_objects/lineage.hpp"

struct i_learn_avoidance {
    virtual ~i_learn_avoidance() = default;
    virtual std::optional<const resolution_lineage*> learn(const lemma&) = 0;
};

#endif

