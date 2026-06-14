#ifndef EXPR_VISITOR_HPP
#define EXPR_VISITOR_HPP

#include <cstdint>
#include <map>
#include <string>
#include "../generated/CHCBaseVisitor.h"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"
#include "interfaces/i_var_sequencer.hpp"

struct expr_visitor : public CHCBaseVisitor {
    expr_visitor(i_make_functor&, i_make_var&, i_var_sequencer&,
                 std::map<std::string, uint32_t>&, std::map<std::string, uint32_t>&, uint32_t&);
    std::any visitExpr(CHCParser::ExprContext*) override;

private:
    const expr* visitVar(antlr4::tree::TerminalNode*);
    std::any visitFunctor(CHCParser::FunctorContext*) override;
    std::any visitList(CHCParser::ListContext*) override;
    uint32_t atom_id(const std::string& name);

    i_make_functor& make_functor;
    i_make_var& make_var;
    i_var_sequencer& var_seq;
    std::map<std::string, uint32_t>& var_map;
    std::map<std::string, uint32_t>& atom_map;
    uint32_t& next_atom_id;
};

#endif
