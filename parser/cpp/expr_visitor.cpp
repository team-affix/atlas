#include "../hpp/expr_visitor.hpp"

expr_visitor::expr_visitor(expr_pool& pool, sequencer& seq, std::map<std::string, uint32_t>& var_map)
    : pool(pool), seq(seq), var_map(var_map) {}

antlrcpp::Any expr_visitor::visitSexp(CHCParser::SexpContext* ctx) {
    if (auto* atom = ctx->ATOM())
        return visitAtom(atom);

    if (auto* var = ctx->VARIABLE())
        return visitVar(var);

    // Parenthesized form: children are ( sexp+ ) or ( sexp [.] sexp ).
    auto sexps = ctx->sexp();
    
    // The dot, if present, is cons.
    if (ctx->children.size() > 2 && ctx->children[2]->getText() == ".")
        return visitCons(sexps);

    // Otherwise, it's a list.
    return visitList(sexps);
}

const expr* expr_visitor::visitAtom(antlr4::tree::TerminalNode* node) {
    return pool.atom(node->getText());
}

const expr* expr_visitor::visitVar(antlr4::tree::TerminalNode* node) {
    const std::string& name = node->getText();
    auto [it, inserted] = var_map.emplace(name, 0u);
    if (inserted) it->second = seq();
    return pool.var(it->second);
}

const expr* expr_visitor::visitCons(std::vector<CHCParser::SexpContext*>& sexps) {
    return pool.cons(std::any_cast<const expr*>(visit(sexps[0])),
                     std::any_cast<const expr*>(visit(sexps[1])));
}

const expr* expr_visitor::visitList(std::vector<CHCParser::SexpContext*>& sexps) {
    const expr* result = pool.atom("nil");
    for (int i = sexps.size() - 1; i >= 0; i--)
        result = pool.cons(std::any_cast<const expr*>(visit(sexps[i])), result);
    return result;
}
