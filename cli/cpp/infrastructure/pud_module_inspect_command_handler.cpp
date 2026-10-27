#include "infrastructure/pud_module_inspect_command_handler.hpp"
#include "infrastructure/hex_uint64.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_module_deserializer_stack.hpp"
#include "value_objects/pud_node_id.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>

pud_module_inspect_command_handler::pud_module_inspect_command_handler(const std::string& input_path)
    : input_path_(input_path) {}

void pud_module_inspect_command_handler::operator()() {
    std::ifstream in(input_path_, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open input file: " + input_path_);
    pud_module_deserializer_stack deserializer_stack;
    const pud_module module = deserializer_stack.deserialize_module(in);
    hex_uint64 hex_codec;
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
}
