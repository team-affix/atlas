#ifndef I_ADVANCE_SOLVER_SESSION_HPP
#define I_ADVANCE_SOLVER_SESSION_HPP

struct i_advance_solver_session {
    virtual ~i_advance_solver_session() = default;
    virtual bool next() = 0;
};

#endif
