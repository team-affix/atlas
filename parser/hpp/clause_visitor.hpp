#ifndef CLAUSE_VISITOR_HPP
#define CLAUSE_VISITOR_HPP

#include <map>
#include <string>
#include "../generated/CHCBaseVisitor.h"
#include "../hpp/expr_visitor.hpp"
#include "../hpp/body_visitor.hpp"
#include "../../core/hpp/rule.hpp"
#include "../../core/hpp/sequencer.hpp"

struct clause_visitor : public CHCBaseVisitor {
    clause_visitor(expr_pool&, sequencer&);
    antlrcpp::Any visitClause(CHCParser::ClauseContext*) override;
#ifndef DEBUG
private:
#endif
    expr_pool& pool;
    sequencer& seq;
};

#endif
