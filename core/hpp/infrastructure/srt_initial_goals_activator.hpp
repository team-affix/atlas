#ifndef SRT_INITIAL_GOALS_ACTIVATOR_HPP
#define SRT_INITIAL_GOALS_ACTIVATOR_HPP

template<typename IFlushSrtGoalBatch, typename IInitialGoalsActivator>
struct srt_initial_goals_activator {
    srt_initial_goals_activator(IFlushSrtGoalBatch&, IInitialGoalsActivator&);
    bool activate_initial_goals_and_candidates();
private:
    IFlushSrtGoalBatch& flush_srt_goal_batch_;
    IInitialGoalsActivator& initial_goals_activator_;
};

template<typename IFSGB, typename IIGA>
srt_initial_goals_activator<IFSGB, IIGA>::srt_initial_goals_activator(IFSGB& sag, IIGA& iga)
    : flush_srt_goal_batch_(sag), initial_goals_activator_(iga) {}

template<typename IFSGB, typename IIGA>
bool srt_initial_goals_activator<IFSGB, IIGA>::activate_initial_goals_and_candidates() {
    if (!initial_goals_activator_.activate_initial_goals_and_candidates()) return false;
    flush_srt_goal_batch_.flush_srt_goal_batch();
    return true;
}

#endif
