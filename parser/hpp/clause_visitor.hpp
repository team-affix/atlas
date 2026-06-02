#ifndef CLAUSE_VISITOR_HPP
#define CLAUSE_VISITOR_HPP

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
    clause_visitor(i_make_functor&, i_make_var&, i_var_sequencer&);
    antlrcpp::Any visitClause(CHCParser::ClauseContext*) override;
#ifndef DEBUG
private:
#endif
    i_make_functor& make_functor;
    i_make_var& make_var;
    i_var_sequencer& var_seq;
};

#endif
