#include "../../hpp/infrastructure/goal_candidate_activator_visitor.hpp"

goal_candidate_activator_visitor::goal_candidate_activator_visitor(
    const goal_lineage* gl,
    i_copier& copier,
    i_bind_map& common_bind_map,
    i_bind_map_factory& bind_map_factory,
    i_overlay_bind_map_factory& overlay_bind_map_factory,
    i_unifier_factory& unifier_factory,
    i_set_candidate_translation_map& set_candidate_translation_map,
    i_mhu_elimination_generator& mhu_elimination_generator,
    i_candidate_activator& candidate_activator,
    i_elimination_backlog& elimination_backlog,
    i_lineage_pool& lp,
    i_get_goal_expr& gge)
    :
    gl(gl),
    copier(copier),
    common_bind_map(common_bind_map),
    bind_map_factory(bind_map_factory),
    overlay_bind_map_factory(overlay_bind_map_factory),
    unifier_factory(unifier_factory),
    set_candidate_translation_map(set_candidate_translation_map),
    mhu_elimination_generator(mhu_elimination_generator),
    candidate_activator(candidate_activator),
    elimination_backlog(elimination_backlog),
    lp(lp),
    gge(gge) {}

void goal_candidate_activator_visitor::visit(const rule* r) {
    const resolution_lineage* rl = lp.resolution(gl, r);

    // 1. if the candidate is eliminated, skip it
    if (elimination_backlog.contains(rl))
        return;

    // 2. early skip if unification fails

    // 2.1. construct the translation map
    translation_map tm;
    const expr* copied_head = copier.copy(r->head, tm);
    
    // 2.1. construct a bind map
    auto bind_map = bind_map_factory.make();
    
    // 2.2. construct an overlay bind map
    auto overlay_bind_map = overlay_bind_map_factory.make(*bind_map, common_bind_map);
    
    // 2.3. construct a unifier on the overlay bind map
    auto unifier = unifier_factory.make(*overlay_bind_map);
    
    // 2.4. get the goal expression
    const expr* goal_expr = gge.get(gl);
    
    // 2.5. construct a rep_changed set
    std::unordered_set<uint32_t> rep_changed;
    
    // 2.4. perform unification
    if (!unifier->unify(gge.get(gl), copied_head, rep_changed))
        return;
    
    // 3. construct a unify_head and add to mhu

    // 3.1. construct a unify_head
    unify_head head{
        std::move(bind_map),
        std::move(overlay_bind_map),
        std::move(unifier)};

    // 3.2. add to mhu
    mhu_elimination_generator.add_head(rl, std::move(head), rep_changed);

    // 3.3. add translation map
    set_candidate_translation_map.set(rl, std::move(tm));
    
    // 4. activate the candidate
    candidate_activator.activate(rl);
    
}
