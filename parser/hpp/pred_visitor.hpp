#ifndef PRED_VISITOR_HPP
#define PRED_VISITOR_HPP

#include <map>
#include <string>
#include "../generated/CHCBaseVisitor.h"
#include "../hpp/expr_visitor.hpp"
#include "../../core/hpp/expr.hpp"
#include "../../core/hpp/sequencer.hpp"

struct pred_visitor : public CHCBaseVisitor {
    pred_visitor(expr_pool&, sequencer&, std::map<std::string, uint32_t>&);
    antlrcpp::Any visitPred(CHCParser::PredContext*) override;
#ifndef DEBUG
private:
#endif
    expr_pool& pool;
    sequencer& seq;
    std::map<std::string, uint32_t>& var_map;
};

#endif
