#ifndef BODY_VISITOR_HPP
#define BODY_VISITOR_HPP

#include <map>
#include <string>
#include <vector>
#include "../generated/CHCBaseVisitor.h"
#include "../hpp/expr_visitor.hpp"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"
#include "interfaces/i_var_sequencer.hpp"

struct body_visitor : public CHCBaseVisitor {
    body_visitor(i_make_functor&, i_make_var&, i_var_sequencer&,
                 std::map<std::string, uint32_t>&, std::map<std::string, uint32_t>&, uint32_t&);
    antlrcpp::Any visitBody(CHCParser::BodyContext*) override;

private:
    i_make_functor& make_functor;
    i_make_var& make_var;
    i_var_sequencer& var_seq;
    std::map<std::string, uint32_t>& var_map;
    std::map<std::string, uint32_t>& functor_map;
    uint32_t& next_functor_id;
};

#endif
