#include "infrastructure/pud_module_inspect_command_handler.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "infrastructure/expr_printer.hpp"
#include "infrastructure/functor_names.hpp"
#include "infrastructure/hex_uint64.hpp"
#include "infrastructure/id_var_printer.hpp"
#include "infrastructure/named_functor_printer.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_module_deserializer_stack.hpp"
#include "infrastructure/rule_printer.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_node_id.hpp"

pud_module_inspect_command_handler::pud_module_inspect_command_handler(
    const std::string& input_path,
    bool id_only,
    bool deps_only,
    bool detailed)
    : input_path_(input_path)
    , id_only_(id_only)
    , deps_only_(deps_only)
    , detailed_(detailed) {}

void pud_module_inspect_command_handler::operator()() {
    std::ifstream in(input_path_, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open input file: " + input_path_);
    pud_module_deserializer_stack deserializer_stack;
    const pud_module module = deserializer_stack.deserialize_module(in);
    hex_uint64 hex_codec;
    if (id_only_) {
        std::cout << hex_codec.format_hex(module.get_module_id()) << '\n';
        return;
    }
    if (deps_only_) {
        const std::vector<pud_module_id>& dependencies = module.get_dependencies();
        for (size_t index = 0; index < dependencies.size(); ++index) {
            if (index > 0)
                std::cout << ' ';
            std::cout << hex_codec.format_hex(dependencies[index]);
        }
        std::cout << '\n';
        return;
    }
    std::cout << "module_id: " << hex_codec.format_hex(module.get_module_id()) << '\n';
    std::cout << "dependencies:";
    for (pud_module_id dependency : module.get_dependencies())
        std::cout << ' ' << hex_codec.format_hex(dependency);
    std::cout << '\n';
    const size_t axiom_count = module.count_axioms();
    const size_t node_count = module.count_nodes();
    const size_t inference_count = node_count - axiom_count;
    size_t refuted_count = 0;
    for (pud_node_id node = 0; node < node_count; ++node) {
        if (module.check_refuted(node))
            ++refuted_count;
    }
    std::cout << "axioms: " << axiom_count << '\n';
    std::cout << "inferences: " << inference_count << '\n';
    std::cout << "nodes: " << node_count << '\n';
    std::cout << "refuted: " << refuted_count << '\n';
    if (detailed_)
        print_nodes(module);
}

void pud_module_inspect_command_handler::print_nodes(const pud_module& module) const {
    functor_names names;
    const std::vector<std::string>& stored_names = module.functor_names();
    for (uint32_t functor_id = 0; functor_id < stored_names.size(); ++functor_id)
        names.set_name(functor_id, stored_names[functor_id]);
    id_var_printer print_var;
    named_functor_printer<functor_names> print_functor{names};
    expr_printer<id_var_printer, named_functor_printer<functor_names>> print_expr{std::cout, print_var, print_functor};
    rule_printer<expr_printer<id_var_printer, named_functor_printer<functor_names>>> print_rule{std::cout, print_expr};
    const size_t axiom_count = module.count_axioms();
    const size_t node_count = module.count_nodes();
    for (pud_node_id node = 0; node < node_count; ++node) {
        std::cout << "node " << node << ' ';
        if (node < axiom_count) {
            std::cout << "axiom ";
            print_rule.print(module.get_axiom(node));
            std::cout << '\n';
            continue;
        }
        const pud_inference& inference = module.get_inference(node);
        std::cout << "inference parent=" << inference.parent
                  << " call_site=" << inference.call_site << '\n';
    }
}
