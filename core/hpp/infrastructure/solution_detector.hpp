#ifndef SOLUTION_DETECTOR_HPP
#define SOLUTION_DETECTOR_HPP

template<typename ICheckActiveGoalsEmpty>
struct solution_detector {
    solution_detector(ICheckActiveGoalsEmpty& ag);
    bool detect() const;
private:
    ICheckActiveGoalsEmpty& check_active_goals_empty_;
};

template<typename ICheckActiveGoalsEmpty>
solution_detector<ICheckActiveGoalsEmpty>::solution_detector(ICheckActiveGoalsEmpty& ag)
    : check_active_goals_empty_(ag) {}

template<typename ICheckActiveGoalsEmpty>
bool solution_detector<ICheckActiveGoalsEmpty>::detect() const {
    return check_active_goals_empty_.empty();
}

#endif
