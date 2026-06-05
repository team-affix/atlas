#ifndef I_GET_SOLVER_SESSION_SOLVED_HPP
#define I_GET_SOLVER_SESSION_SOLVED_HPP

struct i_get_solver_session_solved {
    virtual ~i_get_solver_session_solved() = default;
    virtual bool solved() const = 0;
};

#endif
