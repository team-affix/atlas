#ifndef CLAUSE_VISITOR_HPP
#define CLAUSE_VISITOR_HPP

#include <cstdint>
#include <map>
#include <string>
#include "../generated/CHCBaseVisitor.h"
#include "../hpp/expr_visitor.hpp"
#include "../hpp/body_visitor.hpp"
#include "value_objects/rule.hpp"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"
#include "interfaces/i_var_sequencer.hpp"

struct clause_visitor : public CHCBaseVisitor {
    clause_visitor(i_make_functor&, i_make_var&, i_var_sequencer&,
                   std::map<std::string, uint32_t>&, uint32_t&);
    antlrcpp::Any visitClause(CHCParser::ClauseContext*) override;

private:
    i_make_functor& make_functor;
    i_make_var& make_var;
    i_var_sequencer& var_seq;
    std::map<std::string, uint32_t>& atom_map;
    uint32_t& next_atom_id;
};

#endif
