#ifndef I_BACKTRACKABLE_HPP
#define I_BACKTRACKABLE_HPP

struct i_backtrackable {
    virtual ~i_backtrackable() = default;
    virtual void backtrack() = 0;
};

#endif
