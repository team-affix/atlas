#include "../../hpp/infrastructure/goal_candidate_activator_visitor.hpp"

goal_candidate_activator_visitor::goal_candidate_activator_visitor(
    i_copier& copier,
    i_set_candidate_translation_map& set_candidate_translation_map,
    i_mhu_elimination_generator& mhu_elimination_generator,
    i_candidate_activator& candidate_activator)
    :
    copier(copier),
    set_candidate_translation_map(set_candidate_translation_map),
    mhu_elimination_generator(mhu_elimination_generator),
    candidate_activator(candidate_activator) {}

void goal_candidate_activator_visitor::visit(const rule* r) {
    // 1. construct the translation map
    translation_map tm;
    const expr* copied_head = copier.copy(r->head, tm);
    // 2. construct a unify_head
    
}
