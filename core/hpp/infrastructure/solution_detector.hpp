#ifndef SOLUTION_DETECTOR_HPP
#define SOLUTION_DETECTOR_HPP

template<typename IActiveGoals>
struct solution_detector {
    solution_detector(IActiveGoals& ag);
    bool detect() const;
private:
    IActiveGoals& check_active_goals_empty;
};

template<typename IActiveGoals>
solution_detector<IActiveGoals>::solution_detector(IActiveGoals& ag)
    : check_active_goals_empty(ag) {}

template<typename IActiveGoals>
bool solution_detector<IActiveGoals>::detect() const {
    return check_active_goals_empty.empty();
}

#endif
