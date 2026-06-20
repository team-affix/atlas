#ifndef EXPR_VISITOR_HPP
#define EXPR_VISITOR_HPP

#include <any>
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "../generated/CHCBaseVisitor.h"
#include "infrastructure/functor_names.hpp"
#include "value_objects/expr.hpp"

template<typename IMakeFunctor, typename IMakeVar, typename IVarSequencer>
struct expr_visitor : public CHCBaseVisitor {
    expr_visitor(IMakeFunctor&, IMakeVar&, IVarSequencer&,
                 std::map<std::string, uint32_t>&, std::map<std::string, uint32_t>&, uint32_t&);
    std::any visitExpr(CHCParser::ExprContext*) override;

private:
    const expr* visitVar(antlr4::tree::TerminalNode*);
    std::any visitFunctor(CHCParser::FunctorContext*) override;
    std::any visitList(CHCParser::ListContext*) override;
    uint32_t functor_id(const std::string& name);

    IMakeFunctor& make_functor;
    IMakeVar& make_var;
    IVarSequencer& var_seq;
    std::map<std::string, uint32_t>& var_map;
    std::map<std::string, uint32_t>& functor_map;
    uint32_t& next_functor_id;
};

template<typename IMF, typename IMV, typename IVS>
expr_visitor<IMF,IMV,IVS>::expr_visitor(IMF& make_functor, IMV& make_var, IVS& var_seq,
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
std::any expr_visitor<IMF,IMV,IVS>::visitExpr(CHCParser::ExprContext* ctx) {
    if (auto* var = ctx->VARIABLE())
        return (const expr*)visitVar(var);

    return visitFunctor(ctx->functor());
}

template<typename IMF, typename IMV, typename IVS>
const expr* expr_visitor<IMF,IMV,IVS>::visitVar(antlr4::tree::TerminalNode* node) {
    const std::string& name = node->getText();
    if (name == "_")
        return make_var.make_var(var_seq.next());
    auto [it, inserted] = var_map.emplace(name, 0u);
    if (inserted) it->second = var_seq.next();
    return make_var.make_var(it->second);
}

template<typename IMF, typename IMV, typename IVS>
uint32_t expr_visitor<IMF,IMV,IVS>::functor_id(const std::string& name) {
    auto [it, inserted] = functor_map.emplace(name, 0u);
    if (inserted) it->second = next_functor_id++;
    return it->second;
}

template<typename IMF, typename IMV, typename IVS>
std::any expr_visitor<IMF,IMV,IVS>::visitFunctor(CHCParser::FunctorContext* ctx) {
    if (ctx->list())
        return visitList(ctx->list());

    auto* atom_token = ctx->ATOM();
    assert(atom_token != nullptr);
    std::string name = atom_token->getText();
    std::vector<const expr*> args;
    for (auto* sub_expr : ctx->expr())
        args.push_back(std::any_cast<const expr*>(visit(sub_expr)));
    return (const expr*)make_functor.make_functor(functor_id(name), std::move(args));
}

template<typename IMF, typename IMV, typename IVS>
std::any expr_visitor<IMF,IMV,IVS>::visitList(CHCParser::ListContext* ctx) {
    auto exprs = ctx->expr();

    if (exprs.empty())
        return (const expr*)make_functor.make_functor(k_nil_functor_id, {});

    bool has_pipe = false;
    for (auto* child : ctx->children) {
        if (child->getText() == "|") {
            has_pipe = true;
            break;
        }
    }

    size_t head_count = has_pipe ? exprs.size() - 1 : exprs.size();

    std::vector<const expr*> heads;
    heads.reserve(head_count);
    for (size_t i = 0; i < head_count; ++i)
        heads.push_back(std::any_cast<const expr*>(visit(exprs[i])));

    const expr* tail = has_pipe
        ? std::any_cast<const expr*>(visit(exprs.back()))
        : make_functor.make_functor(k_nil_functor_id, {});

    const expr* result = tail;
    for (int i = static_cast<int>(head_count) - 1; i >= 0; --i)
        result = make_functor.make_functor(k_cons_functor_id, {heads[i], result});
    return (const expr*)result;
}

#endif
