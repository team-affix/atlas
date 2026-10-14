#ifndef REPRESENTATIVE_CHANGE_LOGGER_HPP
#define REPRESENTATIVE_CHANGE_LOGGER_HPP

#include <cstdint>
#include <vector>
#include "value_objects/expr.hpp"

template<typename IEquate, typename IInstantiate, typename IPushBackChangedRep>
struct representative_change_logger {
    representative_change_logger(
        IEquate& equate,
        IInstantiate& instantiate,
        IPushBackChangedRep& push_back_changed_rep);

    void equate(uint32_t rep_a, uint32_t rep_b);
    void instantiate(uint32_t rep, uint32_t functor_id, const std::vector<const expr*>& args);

private:
    IEquate& equate_;
    IInstantiate& instantiate_;
    IPushBackChangedRep& push_back_changed_rep_;
};

template<typename IEquate, typename IInstantiate, typename IPushBackChangedRep>
representative_change_logger<IEquate, IInstantiate, IPushBackChangedRep>::representative_change_logger(
    IEquate& equate,
    IInstantiate& instantiate,
    IPushBackChangedRep& push_back_changed_rep)
    : equate_(equate)
    , instantiate_(instantiate)
    , push_back_changed_rep_(push_back_changed_rep) {}

template<typename IEquate, typename IInstantiate, typename IPushBackChangedRep>
void representative_change_logger<IEquate, IInstantiate, IPushBackChangedRep>::equate(
    uint32_t rep_a, uint32_t rep_b) {
    equate_.equate(rep_a, rep_b);
    // The class with the larger rep changed: it now resolves to the smaller rep.
    push_back_changed_rep_.push_back(rep_a < rep_b ? rep_b : rep_a);
}

template<typename IEquate, typename IInstantiate, typename IPushBackChangedRep>
void representative_change_logger<IEquate, IInstantiate, IPushBackChangedRep>::instantiate(
    uint32_t rep,
    uint32_t functor_id,
    const std::vector<const expr*>& args) {
    instantiate_.instantiate(rep, functor_id, args);
    push_back_changed_rep_.push_back(rep);
}

#endif
