#ifndef TEAR_DOWN_SIM_HPP
#define TEAR_DOWN_SIM_HPP

template<typename ITrail, typename IUnitGoals, typename IDecisionMemory,
         typename IResolutionMemory, typename IGoalCandidateRules, typename IGoalExprs,
         typename IActiveGoals, typename ICandidateFrameOffsets, typename IMhuElimGen,
         typename IBindMap, typename ILineagePool, typename IFrameAllocator,
         typename ICdclElimGen, typename IChosenGoalCandidates>
struct tear_down_sim {
    tear_down_sim(ITrail&, IUnitGoals&, IDecisionMemory&, IResolutionMemory&,
                  IGoalCandidateRules&, IGoalExprs&, IActiveGoals&,
                  ICandidateFrameOffsets&, IMhuElimGen&, IBindMap&,
                  ILineagePool&, IFrameAllocator&, ICdclElimGen&,
                  IChosenGoalCandidates&);
    void tear_down();
private:
    ITrail& pop_trail_frame_;
    IUnitGoals& clear_unit_goals_;
    IDecisionMemory& clear_recorded_decisions_;
    IResolutionMemory& clear_recorded_resolutions_;
    IGoalCandidateRules& clear_goal_candidate_rule_ids_;
    IGoalExprs& clear_goal_exprs_;
    IActiveGoals& clear_active_goals_;
    ICandidateFrameOffsets& clear_candidate_frame_offsets_;
    IMhuElimGen& clear_mhu_heads_;
    IBindMap& clear_bindings_;
    ILineagePool& trim_unpinned_lineages_;
    IFrameAllocator& frame_allocator_;
    ICdclElimGen& clean_up_cdcl_;
    IChosenGoalCandidates& clear_chosen_goal_candidates_;
};

template<typename IT, typename IUG, typename IDM, typename IRM, typename IGCR,
         typename IGE, typename IAG, typename ICFO, typename IMEG, typename IBM,
         typename ILP, typename IFA, typename ICEG, typename ICGC>
tear_down_sim<IT,IUG,IDM,IRM,IGCR,IGE,IAG,ICFO,IMEG,IBM,ILP,IFA,ICEG,ICGC>::tear_down_sim(
    IT& t, IUG& ug, IDM& dm, IRM& rm, IGCR& gcr, IGE& ge, IAG& ag,
    ICFO& cfo, IMEG& meg, IBM& bm, ILP& lp, IFA& fa, ICEG& ceg, ICGC& cgc)
    : pop_trail_frame_(t), clear_unit_goals_(ug), clear_recorded_decisions_(dm),
      clear_recorded_resolutions_(rm), clear_goal_candidate_rule_ids_(gcr),
      clear_goal_exprs_(ge), clear_active_goals_(ag), clear_candidate_frame_offsets_(cfo),
      clear_mhu_heads_(meg), clear_bindings_(bm), trim_unpinned_lineages_(lp),
      frame_allocator_(fa), clean_up_cdcl_(ceg), clear_chosen_goal_candidates_(cgc) {}

template<typename IT, typename IUG, typename IDM, typename IRM, typename IGCR,
         typename IGE, typename IAG, typename ICFO, typename IMEG, typename IBM,
         typename ILP, typename IFA, typename ICEG, typename ICGC>
void tear_down_sim<IT,IUG,IDM,IRM,IGCR,IGE,IAG,ICFO,IMEG,IBM,ILP,IFA,ICEG,ICGC>::tear_down() {
    pop_trail_frame_.pop();
    clear_unit_goals_.clear();
    clear_recorded_decisions_.clear_recorded_decisions();
    clear_recorded_resolutions_.clear_recorded_resolutions();
    clear_goal_candidate_rule_ids_.clear_goal_candidate_rule_ids();
    clear_goal_exprs_.clear_goal_exprs();
    clear_active_goals_.clear_active_goals();
    clear_candidate_frame_offsets_.clear_candidate_frame_offsets();
    clear_mhu_heads_.clear_mhu_heads();
    clear_bindings_.clear_bindings();
    frame_allocator_.reset();
    clean_up_cdcl_.cleanup();
    clear_chosen_goal_candidates_.clear();
    trim_unpinned_lineages_.trim();
}

#endif
