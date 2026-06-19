#ifndef I_CLEAN_UP_CDCL_HPP
#define I_CLEAN_UP_CDCL_HPP

struct i_clean_up_cdcl {
    virtual ~i_clean_up_cdcl() = default;
    virtual void cleanup() = 0;
};

#endif
