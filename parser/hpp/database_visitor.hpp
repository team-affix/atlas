#ifndef DATABASE_VISITOR_HPP
#define DATABASE_VISITOR_HPP

#include <cstdint>
#include <map>
#include <string>
#include "../generated/CHCBaseVisitor.h"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"
#include "interfaces/i_var_sequencer.hpp"
#include "interfaces/i_push_db_rule.hpp"

struct database_visitor : public CHCBaseVisitor {
    database_visitor(i_make_functor&, i_make_var&, i_var_sequencer&, i_push_db_rule&,
                     std::map<std::string, uint32_t>&, uint32_t&);
    antlrcpp::Any visitDatabase(CHCParser::DatabaseContext*) override;

private:
    i_make_functor& make_functor;
    i_make_var& make_var;
    i_var_sequencer& var_seq;
    i_push_db_rule& out;
    std::map<std::string, uint32_t>& atom_map;
    uint32_t& next_atom_id;
};

#endif
