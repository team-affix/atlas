#ifndef CLAUSE_VISITOR_HPP
#define CLAUSE_VISITOR_HPP

#include <any>
#include <cstdint>
#include <map>
#include <string>
#include "../generated/CHCBaseVisitor.h"
#include "../hpp/expr_visitor.hpp"
#include "../hpp/body_visitor.hpp"
#include "value_objects/rule.hpp"
#include "infrastructure/non_backtracking_var_sequencer.hpp"

template<typename IMakeFunctor, typename IMakeVar>
struct clause_visitor : public CHCBaseVisitor {
    clause_visitor(IMakeFunctor&, IMakeVar&,
                   std::map<std::string, uint32_t>&, uint32_t&);
    antlrcpp::Any visitClause(CHCParser::ClauseContext*) override;

private:
    IMakeFunctor& make_functor;
    IMakeVar& make_var;
    std::map<std::string, uint32_t>& functor_map;
    uint32_t& next_functor_id;
};

template<typename IMF, typename IMV>
clause_visitor<IMF,IMV>::clause_visitor(IMF& make_functor, IMV& make_var,
                                        std::map<std::string, uint32_t>& functor_map,
                                        uint32_t& next_functor_id)
    : make_functor(make_functor),
      make_var(make_var),
      functor_map(functor_map),
      next_functor_id(next_functor_id) {}

template<typename IMF, typename IMV>
antlrcpp::Any clause_visitor<IMF,IMV>::visitClause(CHCParser::ClauseContext* ctx) {
    std::map<std::string, uint32_t> var_map;
    non_backtracking_var_sequencer local_seq(0);
    expr_visitor ev(make_functor, make_var, local_seq, var_map, functor_map, next_functor_id);

    const expr* head = std::any_cast<const expr*>(ev.visitExpr(ctx->expr()));

    auto* b = ctx->body();
    if (!b) return rule{head, {}, local_seq.peek()};

    body_visitor bv(make_functor, make_var, local_seq, var_map, functor_map, next_functor_id);
    auto body = std::any_cast<std::vector<const expr*>>(bv.visitBody(b));
    return rule{head, std::move(body), local_seq.peek()};
}

#endif
