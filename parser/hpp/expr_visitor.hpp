#ifndef EXPR_VISITOR_HPP
#define EXPR_VISITOR_HPP

#include <cstdint>
#include <map>
#include <string>
#include "../generated/CHCBaseVisitor.h"
#include "../../core/hpp/expr.hpp"
#include "../../core/hpp/sequencer.hpp"

struct expr_visitor : public CHCBaseVisitor {
    expr_visitor(expr_pool&, sequencer&, std::map<std::string, uint32_t>&);
    std::any visitExpr(CHCParser::ExprContext*) override;
#ifndef DEBUG
private:
#endif
    const expr* visitVar(antlr4::tree::TerminalNode*);
    std::any visitFunctor(CHCParser::FunctorContext*) override;
    std::any visitList(CHCParser::ListContext*) override;

    expr_pool& pool;
    sequencer& seq;
    std::map<std::string, uint32_t>& var_map;
};

#endif
