#ifndef CUMULATIVE_GROUNDED_WEIGHT_HPP
#define CUMULATIVE_GROUNDED_WEIGHT_HPP

struct cumulative_grounded_weight {
    cumulative_grounded_weight();
    void accumulate(double w);
    double get() const;
    void clear();
private:
    double value_;
};

#endif
