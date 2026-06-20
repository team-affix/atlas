#include "infrastructure/horizon_command_handler.hpp"
#include "parser/hpp/import_database_from_file.hpp"
#include "parser/hpp/import_goals_from_string.hpp"
#include <iostream>

horizon_command_handler::horizon_command_handler(
    const std::string& file,
    const std::string& goals_str,
    size_t max_resolutions,
    uint32_t seed,
    double exploration_constant,
    size_t sim_progress_interval)
    : parse_var_seq_(0),
      solve_loop_(print_bindings_, print_progress_, sim_progress_interval) {
    parse_pool_.emplace();
    printer_.emplace(std::cout, var_names_, functor_names_);

    import_database_from_file(
        file, *parse_pool_, *parse_pool_, database_, functor_map_, next_functor_id_);
    var_name_to_idx_ = import_goals_from_string(
        goals_str, *parse_pool_, *parse_pool_, parse_var_seq_, initial_goals_, functor_map_, next_functor_id_);
    const uint32_t initial_frame_offset = parse_var_seq_.peek();
    for (const auto& [name, idx] : var_name_to_idx_)
        var_names_.set_name(idx, name);
    for (const auto& [name, id] : functor_map_)
        functor_names_.set_name(id, name);

    runtime_.emplace(
        database_, initial_goals_, initial_frame_offset, max_resolutions, seed, exploration_constant);
}

void horizon_command_handler::operator()() {
    solve_loop_.run(*runtime_, *printer_, *parse_pool_, var_name_to_idx_);
}
