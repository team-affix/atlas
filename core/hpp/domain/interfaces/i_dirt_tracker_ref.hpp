#ifndef I_DIRT_TRACKER_REF_HPP
#define I_DIRT_TRACKER_REF_HPP

template<typename T>
struct i_dirt_tracker_ref {
    virtual ~i_dirt_tracker_ref() = default;
    T& operator*() = 0;
    const T& operator*() const = 0;
};

#endif
