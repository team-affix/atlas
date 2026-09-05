#ifndef NAMED_FUNCTOR_PRINTER_HPP
#define NAMED_FUNCTOR_PRINTER_HPP

#include <cstdint>
#include <ostream>

template<typename IFunctorNames>
struct named_functor_printer {
    named_functor_printer(IFunctorNames& functor_names);
    void print(std::ostream&, uint32_t id) const;
private:
    IFunctorNames& functor_names_;
};

template<typename IFunctorNames>
named_functor_printer<IFunctorNames>::named_functor_printer(IFunctorNames& functor_names)
    : functor_names_(functor_names) {}

template<typename IFunctorNames>
void named_functor_printer<IFunctorNames>::print(std::ostream& os, uint32_t id) const {
    if (functor_names_.is_named(id)) {
        os << functor_names_.name(id);
        return;
    }
    os << "!" << id;
}

#endif
