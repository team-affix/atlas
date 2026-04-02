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
    antlrcpp::Any visitSexp(CHCParser::SexpContext*) override;
#ifndef DEBUG
private:
#endif
    const expr* visitAtom(antlr4::tree::TerminalNode*);
    const expr* visitVar(antlr4::tree::TerminalNode*);
    const expr* visitCons(std::vector<CHCParser::SexpContext*>&);
    const expr* visitList(std::vector<CHCParser::SexpContext*>&);

    expr_pool& pool;
    sequencer& seq;
    std::map<std::string, uint32_t>& var_map;
};

#endif
