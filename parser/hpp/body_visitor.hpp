#ifndef BODY_VISITOR_HPP
#define BODY_VISITOR_HPP

#include <any>
#include <map>
#include <string>
#include <vector>
#include "../generated/CHCBaseVisitor.h"
#include "../hpp/expr_visitor.hpp"

template<typename IMakeFunctor, typename IMakeVar, typename IVarSequencer>
struct body_visitor : public CHCBaseVisitor {
    body_visitor(IMakeFunctor&, IMakeVar&, IVarSequencer&,
                 std::map<std::string, uint32_t>&, std::map<std::string, uint32_t>&, uint32_t&);
    antlrcpp::Any visitBody(CHCParser::BodyContext*) override;

private:
    IMakeFunctor& make_functor;
    IMakeVar& make_var;
    IVarSequencer& var_seq;
    std::map<std::string, uint32_t>& var_map;
    std::map<std::string, uint32_t>& functor_map;
    uint32_t& next_functor_id;
};

template<typename IMF, typename IMV, typename IVS>
body_visitor<IMF,IMV,IVS>::body_visitor(IMF& make_functor, IMV& make_var, IVS& var_seq,
                                        std::map<std::string, uint32_t>& var_map,
                                        std::map<std::string, uint32_t>& functor_map,
                                        uint32_t& next_functor_id)
    : make_functor(make_functor),
      make_var(make_var),
      var_seq(var_seq),
      var_map(var_map),
      functor_map(functor_map),
      next_functor_id(next_functor_id) {}

template<typename IMF, typename IMV, typename IVS>
antlrcpp::Any body_visitor<IMF,IMV,IVS>::visitBody(CHCParser::BodyContext* ctx) {
    std::vector<const expr*> result;
    expr_visitor ev(make_functor, make_var, var_seq, var_map, functor_map, next_functor_id);
    for (auto* e : ctx->expr())
        result.push_back(std::any_cast<const expr*>(ev.visit(e)));
    return result;
}

#endif
