#include "infrastructure/pud_module_create_command_handler.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/hex_uint64.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_module_serializer_stack.hpp"
#include "parser/hpp/import_database_from_file.hpp"
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>

pud_module_create_command_handler::pud_module_create_command_handler(
    const std::string& database_path,
    std::vector<pud_module_id> dependencies,
    const std::string& output_path)
    : database_path_(database_path)
    , output_path_(output_path)
    , dependencies_(std::move(dependencies))
    , next_functor_id_(k_first_user_functor_id)
    , expr_pool_()
    , database_() {}

void pud_module_create_command_handler::operator()() {
    import_database_from_file(
        database_path_,
        expr_pool_,
        expr_pool_,
        database_,
        functor_map_,
        next_functor_id_);
    std::random_device random_device;
    std::mt19937_64 generator(random_device());
    const pud_module_id module_id = generator();
    std::vector<rule> axioms;
    coroutine<rule_id, void> listed = database_.lookup_all_rules().iterate();
    while (auto rule_id = listed.next())
        axioms.push_back(*database_.get_rule(*rule_id));
    const pud_module module{module_id, dependencies_, std::move(axioms)};
    std::ofstream out(output_path_, std::ios::binary);
    if (!out)
        throw std::runtime_error("cannot open output file: " + output_path_);
    pud_module_serializer_stack serializer_stack;
    serializer_stack.serialize_module(out, module);
    if (!out)
        throw std::runtime_error("failed writing module file: " + output_path_);
    hex_uint64 hex_codec;
    std::cout << hex_codec.format_hex(module_id) << '\n';
}
