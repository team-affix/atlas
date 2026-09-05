#ifndef UNFOLD_COMMAND_HANDLER_HPP
#define UNFOLD_COMMAND_HANDLER_HPP

#include <cstdint>
#include <map>
#include <ostream>
#include <string>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/functor_names.hpp"
#include "infrastructure/var_names.hpp"
#include "value_objects/lineage.hpp"

// CLI handler for `unfold`: loads a basis DB and a derivation DB and unfolds
// one derivation rule at a body index. Default output is the original rule,
// a `---` separator, then each unfolded rule. `--print-db` prints the mutated
// derivation DB. `--overwrite` writes that DB back to the derivation file.
struct unfold_command_handler {
    unfold_command_handler(
        const std::string& basis,
        const std::string& derivation,
        rule_id subject_id,
        subgoal_id body_idx,
        bool print_db,
        bool overwrite);

    void operator()();

private:
    void print_mutated_db(std::ostream& os);

    var_names var_names_;
    functor_names functor_names_;
    std::map<std::string, uint32_t> functor_map_;
    uint32_t next_functor_id_;
    expr_pool parse_pool_;
    db original_db_;
    db expanded_db_;
    std::string derivation_path_;
    rule_id subject_id_;
    subgoal_id body_idx_;
    bool print_db_;
    bool overwrite_;
};

#endif
