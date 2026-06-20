#ifndef DATABASE_VISITOR_HPP
#define DATABASE_VISITOR_HPP

#include <any>
#include <cstdint>
#include <map>
#include <string>
#include "../generated/CHCBaseVisitor.h"
#include "../hpp/clause_visitor.hpp"

template<typename IMakeFunctor, typename IMakeVar, typename IPushDbRule>
struct database_visitor : public CHCBaseVisitor {
    database_visitor(IMakeFunctor&, IMakeVar&, IPushDbRule&,
                     std::map<std::string, uint32_t>&, uint32_t&);
    antlrcpp::Any visitDatabase(CHCParser::DatabaseContext*) override;

private:
    IMakeFunctor& make_functor;
    IMakeVar& make_var;
    IPushDbRule& out;
    std::map<std::string, uint32_t>& functor_map;
    uint32_t& next_functor_id;
};

template<typename IMF, typename IMV, typename IPDR>
database_visitor<IMF,IMV,IPDR>::database_visitor(IMF& make_functor, IMV& make_var,
                                                  IPDR& out,
                                                  std::map<std::string, uint32_t>& functor_map,
                                                  uint32_t& next_functor_id)
    : make_functor(make_functor),
      make_var(make_var),
      out(out),
      functor_map(functor_map),
      next_functor_id(next_functor_id) {}

template<typename IMF, typename IMV, typename IPDR>
antlrcpp::Any database_visitor<IMF,IMV,IPDR>::visitDatabase(CHCParser::DatabaseContext* ctx) {
    clause_visitor cv(make_functor, make_var, functor_map, next_functor_id);
    for (auto* c : ctx->clause())
        out.push(std::any_cast<rule>(cv.visitClause(c)));
    return antlrcpp::Any();
}

#endif
