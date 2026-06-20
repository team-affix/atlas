#ifndef PRINT_BINDINGS_HPP
#define PRINT_BINDINGS_HPP

#include <iostream>
#include <map>
#include <string>
#include "infrastructure/expr_pool.hpp"

template<typename IRuntime, typename IExprPrinter>
struct print_bindings {
    void print(IRuntime&, IExprPrinter&, expr_pool&, const std::map<std::string, uint32_t>&);
};

template<typename IR, typename IEP>
void print_bindings<IR,IEP>::print(
    IR& runtime, IEP& printer, expr_pool& pool,
    const std::map<std::string, uint32_t>& var_name_to_idx) {
    for (const auto& [name, idx] : var_name_to_idx) {
        std::cout << "  " << name << " = ";
        printer.print(runtime.normalize({pool.make_var(idx), 0}));
        std::cout << "\n";
    }
}

#endif
