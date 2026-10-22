#ifndef PUD_RANDOM_SOLVER_HPP
#define PUD_RANDOM_SOLVER_HPP

#include <cstdint>
#include <set>
#include <unordered_map>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_derivation_id.hpp"
#include "value_objects/pud_derivation_id_hash.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_solution.hpp"
#include "value_objects/pud_solver_step.hpp"
#include "value_objects/pud_expansion_choice.hpp"

template<typename IAddConjecture, typename IExpand, typename IChooseExpansion>
struct pud_random_solver {
    pud_random_solver(
        IAddConjecture& add_conjecture,
        IExpand& expand,
        IChooseExpansion& choose_expansion);
    size_t add_conjecture(const expr* head, const initial_goal_exprs& goals, uint32_t var_count);
    coroutine<pud_solver_step, void> solve();
private:
    void track(const pud_frontier_change& change, size_t conjecture);

    IAddConjecture& add_conjecture_;
    IExpand& expand_;
    IChooseExpansion& choose_expansion_;
    size_t conjecture_count_;
    std::set<pud_derivation_id> frontier_;
    std::set<pud_derivation_id> unreported_proven_;
    std::unordered_map<pud_derivation_id, size_t, pud_derivation_id_hash> conjectures_;
};

template<typename IAC, typename IE, typename ICU>
pud_random_solver<IAC, IE, ICU>::pud_random_solver(
    IAC& add_conjecture,
    IE& expand,
    ICU& choose_expansion)
    : add_conjecture_(add_conjecture)
    , expand_(expand)
    , choose_expansion_(choose_expansion)
    , conjecture_count_(0)
    , frontier_()
    , unreported_proven_()
    , conjectures_() {}

template<typename IAC, typename IE, typename ICU>
size_t pud_random_solver<IAC, IE, ICU>::add_conjecture(
    const expr* head, const initial_goal_exprs& goals, uint32_t var_count) {
    const size_t conjecture = conjecture_count_++;
    track(add_conjecture_.add_conjecture(head, goals, var_count), conjecture);
    return conjecture;
}

template<typename IAC, typename IE, typename ICU>
coroutine<pud_solver_step, void> pud_random_solver<IAC, IE, ICU>::solve() {
    while (true) {
        if (!unreported_proven_.empty()) {
            const pud_derivation_id derivation = *unreported_proven_.begin();
            unreported_proven_.erase(unreported_proven_.begin());
            const size_t conjecture = conjectures_.at(derivation);
            conjectures_.erase(derivation);
            co_yield pud_solution{.conjecture = conjecture, .derivation = derivation};
            continue;
        }
        if (frontier_.empty())
            co_return;
        const pud_expansion_choice choice = choose_expansion_.choose_expansion(frontier_);
        const size_t conjecture = conjectures_.at(choice.derivation);
        frontier_.erase(choice.derivation);
        conjectures_.erase(choice.derivation);
        track(expand_.expand(choice.derivation, choice.goal), conjecture);
        co_yield choice;
    }
}

template<typename IAC, typename IE, typename ICU>
void pud_random_solver<IAC, IE, ICU>::track(const pud_frontier_change& change, size_t conjecture) {
    for (pud_derivation_id derivation : change.opened) {
        frontier_.insert(derivation);
        conjectures_.emplace(derivation, conjecture);
    }
    for (pud_derivation_id derivation : change.proven) {
        unreported_proven_.insert(derivation);
        conjectures_.emplace(derivation, conjecture);
    }
    for (pud_derivation_id derivation : change.refuted) {
        frontier_.erase(derivation);
        unreported_proven_.erase(derivation);
        conjectures_.erase(derivation);
    }
}

#endif
