#ifndef RESTORABLE_SET_BASE_HPP
#define RESTORABLE_SET_BASE_HPP

#include "../domain/interfaces/i_restorable_set.hpp"

template<typename S>
struct restorable_set_base : i_restorable_set<S> {
    void restore() override;
    void insert(const S::value_type&) override;
    void erase(const S::value_type&) override;
    const S& get() const override;
#ifndef DEBUG
private:
#endif
    S current;
    S additions;
    S subtractions;
};

template<typename S>
void restorable_set_base<S>::restore() {
    current.erase(additions.begin(), additions.end());
    current.insert(subtractions.begin(), subtractions.end());
}

template<typename S>
void restorable_set_base<S>::insert(const S::value_type& value) {
    auto [_, inserted] = current.insert(value);
    if (!inserted) return;
    auto [_, subtraction_erased] = subtractions.erase(value);
    if (subtraction_erased) return;
    additions.insert(value);
}

template<typename S>
void restorable_set_base<S>::erase(const S::value_type& value) {
    auto [_, erased] = current.erase(value);
    if (!erased) return;
    auto [_, addition_erased] = additions.erase(value);
    if (addition_erased) return;
    subtractions.insert(value);
}

template<typename S>
const S& restorable_set_base<S>::get() const {
    return current;
}

#endif
