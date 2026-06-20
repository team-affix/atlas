#ifndef INITIAL_GOAL_WEIGHT_HPP
#define INITIAL_GOAL_WEIGHT_HPP

struct initial_goal_weight {
    explicit initial_goal_weight(double weight);
    double get() const;
private:
    double weight_;
};

#endif
