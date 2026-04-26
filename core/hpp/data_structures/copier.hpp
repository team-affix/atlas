#ifndef COPIER_HPP
#define COPIER_HPP

#include <map>
#include "sequencer.hpp"
#include "expr.hpp"

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
