#ifndef COPIER_HPP
#define COPIER_HPP

#include <map>
#include "../value_objects/expr.hpp"
#include "../../infrastructure/sequencer.hpp"
#include "expr_pool.hpp"

struct copier {
    copier(sequencer&, expr_pool&);
    const expr* operator()(const expr*, std::map<uint32_t, uint32_t>&);
#ifndef DEBUG
private:
#endif
    sequencer& sequencer_ref;
    expr_pool& expr_pool_ref;
};

#endif
