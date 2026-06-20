#ifndef SRT_INITIAL_GOALS_ACTIVATOR_HPP
#define SRT_INITIAL_GOALS_ACTIVATOR_HPP

template<typename ISrtActiveGoals, typename IInitialGoalsActivator>
struct srt_initial_goals_activator {
    srt_initial_goals_activator(ISrtActiveGoals&, IInitialGoalsActivator&);
    bool activate_initial_goals_and_candidates();
private:
    ISrtActiveGoals& flush_srt_goal_batch_;
    IInitialGoalsActivator& initial_goals_activator_;
};

template<typename ISAG, typename IIGA>
srt_initial_goals_activator<ISAG, IIGA>::srt_initial_goals_activator(ISAG& sag, IIGA& iga)
    : flush_srt_goal_batch_(sag), initial_goals_activator_(iga) {}

template<typename ISAG, typename IIGA>
bool srt_initial_goals_activator<ISAG, IIGA>::activate_initial_goals_and_candidates() {
    if (!initial_goals_activator_.activate_initial_goals_and_candidates()) return false;
    flush_srt_goal_batch_.flush_srt_goal_batch();
    return true;
}

#endif
