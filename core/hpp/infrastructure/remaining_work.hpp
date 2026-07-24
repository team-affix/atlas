#ifndef REMAINING_WORK_HPP
#define REMAINING_WORK_HPP

struct remaining_work {
    remaining_work();
    void add(double w);
    void subtract(double w);
    double get() const;
    void clear();
private:
    double value_;
};

#endif
