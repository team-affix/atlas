#include "infrastructure/pud_module_page_command_handler.hpp"
#include <fstream>
#include <stdexcept>
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_module_deserializer_stack.hpp"
#include "infrastructure/pud_module_pager.hpp"

pud_module_page_command_handler::pud_module_page_command_handler(
    const std::string& input_path, const std::string& output_directory)
    : input_path_(input_path)
    , output_directory_(output_directory) {}

void pud_module_page_command_handler::operator()() {
    std::ifstream in(input_path_, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open input file: " + input_path_);
    pud_module_deserializer_stack deserializer_stack;
    const pud_module module = deserializer_stack.deserialize_module(in);
    pud_module_pager{}.page(module, output_directory_, deserializer_stack.exprs());
}
