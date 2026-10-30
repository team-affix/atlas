#ifndef GLOBAL_FUNCTOR_NAME_PRINTER_HPP
#define GLOBAL_FUNCTOR_NAME_PRINTER_HPP

#include <cstdint>
#include <ostream>

template<typename IGetFunctorName>
struct global_functor_name_printer {
    global_functor_name_printer(IGetFunctorName& get_functor_name);
    void print(std::ostream&, uint32_t id) const;
private:
    IGetFunctorName& get_functor_name_;
};

template<typename IGetFunctorName>
global_functor_name_printer<IGetFunctorName>::global_functor_name_printer(IGetFunctorName& get_functor_name)
    : get_functor_name_(get_functor_name) {}

template<typename IGetFunctorName>
void global_functor_name_printer<IGetFunctorName>::print(std::ostream& os, uint32_t id) const {
    os << get_functor_name_.functor_name(id);
}

#endif
