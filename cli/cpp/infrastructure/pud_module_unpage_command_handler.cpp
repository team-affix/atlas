#include "infrastructure/pud_module_unpage_command_handler.hpp"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_module_pager.hpp"
#include "infrastructure/pud_module_serializer_stack.hpp"
#include "infrastructure/pud_paged_module_assembly.hpp"

namespace {

constexpr size_t k_unpage_cache_bytes = size_t{1} << 26;

}

pud_module_unpage_command_handler::pud_module_unpage_command_handler(
    const std::string& input_directory, const std::string& output_path)
    : input_directory_(input_directory)
    , output_path_(output_path) {}

// Writes to a temporary file first and renames it over the output, as saving does.
void pud_module_unpage_command_handler::operator()() {
    if (!std::filesystem::is_directory(input_directory_))
        throw std::runtime_error("not a paged module directory: " + input_directory_);
    expr_pool exprs;
    pud_paged_module_assembly assembly(input_directory_, exprs, k_unpage_cache_bytes);
    const pud_module module = pud_module_pager{}.unpage(assembly.module());
    const std::string temporary = output_path_ + ".tmp";
    {
        std::ofstream out(temporary, std::ios::binary);
        if (!out)
            throw std::runtime_error("cannot open output file: " + temporary);
        pud_module_serializer_stack{}.serialize_module(out, module);
    }
    std::filesystem::rename(temporary, output_path_);
}
