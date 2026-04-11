#ifndef TRACKED_HPP
#define TRACKED_HPP

#include "trail.hpp"

template <typename T>
struct tracked {
    tracked(trail& t, const T& value) : t(t), value(value) {}
    void mutate(std::function<void(T&)> fwd, std::function<void(T&)> bwd) {
        fwd(value);
        t.log([this, bwd]{bwd(value);}, [this, fwd]{fwd(value);});
    }
    const T& get() const { return value; }
#ifndef DEBUG
private:
#endif
    trail& t;
    T value;
};

#endif
