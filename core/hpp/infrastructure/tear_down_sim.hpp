#ifndef TEAR_DOWN_SIM_HPP
#define TEAR_DOWN_SIM_HPP

template<typename IPopFrame, typename IClearUnitGoals, typename IClearRecordedDecisions,
         typename IClearRecordedResolutions, typename IClearGoalCandidateRuleIds, typename IClearGoalExprs,
         typename IClearActiveGoals, typename IClearCandidateFrameOffsets, typename IClearMhuHeads,
         typename IClearBindings, typename ITrimUnpinnedLineages, typename IResetFrameAllocator,
         typename ICleanUpCdcl, typename IClearChosenGoalCandidates>
struct tear_down_sim {
    tear_down_sim(IPopFrame&, IClearUnitGoals&, IClearRecordedDecisions&, IClearRecordedResolutions&,
                  IClearGoalCandidateRuleIds&, IClearGoalExprs&, IClearActiveGoals&,
                  IClearCandidateFrameOffsets&, IClearMhuHeads&, IClearBindings&,
                  ITrimUnpinnedLineages&, IResetFrameAllocator&, ICleanUpCdcl&,
                  IClearChosenGoalCandidates&);
    void tear_down();
private:
    IPopFrame& pop_frame_;
    IClearUnitGoals& clear_unit_goals_;
    IClearRecordedDecisions& clear_recorded_decisions_;
    IClearRecordedResolutions& clear_recorded_resolutions_;
    IClearGoalCandidateRuleIds& clear_goal_candidate_rule_ids_;
    IClearGoalExprs& clear_goal_exprs_;
    IClearActiveGoals& clear_active_goals_;
    IClearCandidateFrameOffsets& clear_candidate_frame_offsets_;
    IClearMhuHeads& clear_mhu_heads_;
    IClearBindings& clear_bindings_;
    ITrimUnpinnedLineages& trim_unpinned_lineages_;
    IResetFrameAllocator& frame_allocator_;
    ICleanUpCdcl& clean_up_cdcl_;
    IClearChosenGoalCandidates& clear_chosen_goal_candidates_;
};

template<typename IPTF, typename ICUG, typename ICRD, typename ICRR, typename ICGCRI,
         typename ICGE, typename ICAG, typename ICCFO, typename ICMH, typename ICB,
         typename ITUL, typename IRFA, typename ICUC, typename ICCGC>
tear_down_sim<IPTF,ICUG,ICRD,ICRR,ICGCRI,ICGE,ICAG,ICCFO,ICMH,ICB,ITUL,IRFA,ICUC,ICCGC>::tear_down_sim(
    IPTF& t, ICUG& ug, ICRD& dm, ICRR& rm, ICGCRI& gcr, ICGE& ge, ICAG& ag,
    ICCFO& cfo, ICMH& meg, ICB& bm, ITUL& lp, IRFA& fa, ICUC& ceg, ICCGC& cgc)
    : pop_frame_(t), clear_unit_goals_(ug), clear_recorded_decisions_(dm),
      clear_recorded_resolutions_(rm), clear_goal_candidate_rule_ids_(gcr),
      clear_goal_exprs_(ge), clear_active_goals_(ag), clear_candidate_frame_offsets_(cfo),
      clear_mhu_heads_(meg), clear_bindings_(bm), trim_unpinned_lineages_(lp),
      frame_allocator_(fa), clean_up_cdcl_(ceg), clear_chosen_goal_candidates_(cgc) {}

template<typename IPTF, typename ICUG, typename ICRD, typename ICRR, typename ICGCRI,
         typename ICGE, typename ICAG, typename ICCFO, typename ICMH, typename ICB,
         typename ITUL, typename IRFA, typename ICUC, typename ICCGC>
void tear_down_sim<IPTF,ICUG,ICRD,ICRR,ICGCRI,ICGE,ICAG,ICCFO,ICMH,ICB,ITUL,IRFA,ICUC,ICCGC>::tear_down() {
    pop_frame_.pop_frame();
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
