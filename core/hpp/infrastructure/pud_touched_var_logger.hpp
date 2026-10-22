#ifndef PUD_TOUCHED_VAR_LOGGER_HPP
#define PUD_TOUCHED_VAR_LOGGER_HPP

#include <cstdint>
#include "value_objects/framed_expr.hpp"

template<typename IEquate, typename IInstantiate, typename IPushBackTouchedVar>
struct pud_touched_var_logger {
    pud_touched_var_logger(
        IEquate& equate,
        IInstantiate& instantiate,
        IPushBackTouchedVar& push_back_touched_var);
    void equate(uint32_t var_a, uint32_t var_b);
    void instantiate(uint32_t var, framed_expr value);
private:
    IEquate& equate_;
    IInstantiate& instantiate_;
    IPushBackTouchedVar& push_back_touched_var_;
};

template<typename IE, typename II, typename IPBTV>
pud_touched_var_logger<IE, II, IPBTV>::pud_touched_var_logger(
    IE& equate,
    II& instantiate,
    IPBTV& push_back_touched_var)
    : equate_(equate)
    , instantiate_(instantiate)
    , push_back_touched_var_(push_back_touched_var) {}

template<typename IE, typename II, typename IPBTV>
void pud_touched_var_logger<IE, II, IPBTV>::equate(uint32_t var_a, uint32_t var_b) {
    equate_.equate(var_a, var_b);
    push_back_touched_var_.push_back(var_a);
    push_back_touched_var_.push_back(var_b);
}

template<typename IE, typename II, typename IPBTV>
void pud_touched_var_logger<IE, II, IPBTV>::instantiate(uint32_t var, framed_expr value) {
    instantiate_.instantiate(var, value);
    push_back_touched_var_.push_back(var);
}

#endif
