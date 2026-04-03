#ifndef BODY_VISITOR_HPP
#define BODY_VISITOR_HPP

#include <map>
#include <string>
#include <vector>
#include "../generated/CHCBaseVisitor.h"
#include "../hpp/expr_visitor.hpp"
#include "../../core/hpp/expr.hpp"
#include "../../core/hpp/sequencer.hpp"

struct body_visitor : public CHCBaseVisitor {
    body_visitor(expr_pool&, sequencer&, std::map<std::string, uint32_t>&);
    antlrcpp::Any visitBody(CHCParser::BodyContext*) override;
#ifndef DEBUG
private:
#endif
    expr_pool& pool;
    sequencer& seq;
    std::map<std::string, uint32_t>& var_map;
};

#endif
