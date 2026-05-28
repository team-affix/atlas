#ifndef I_TRIM_UNPINNED_LINEAGES_HPP
#define I_TRIM_UNPINNED_LINEAGES_HPP

struct i_trim_unpinned_lineages {
    virtual ~i_trim_unpinned_lineages() = default;
    virtual void trim() = 0;
};

#endif
