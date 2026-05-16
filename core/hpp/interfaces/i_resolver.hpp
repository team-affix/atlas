#ifndef I_RESOLVER_HPP
#define I_RESOLVER_HPP

#include <optional>
#include "../value_objects/lineage.hpp"
#include "../value_objects/sim_termination.hpp"

struct i_resolver {
    virtual ~i_resolver() = default;
    virtual std::optional<sim_termination> resolve(const resolution_lineage*) = 0;
};

#endif
