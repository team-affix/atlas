#ifndef I_LOGGER_HPP
#define I_LOGGER_HPP

#include <ostream>

struct i_logger {
    virtual ~i_logger() = default;
    virtual std::ostream& get_ostream() = 0;
};

#endif
