#ifndef TRAIL_SAVEPOINT_HPP
#define TRAIL_SAVEPOINT_HPP

// RAII savepoint over a trail-like frame controller (push / pop / squash_one).
//
// Construction pushes a temp frame. commit() squashes that frame into the frame
// below it, keeping every record logged since construction (they become part of
// the enclosing frame). If the savepoint is destroyed WITHOUT commit(), it pops
// the frame, unwinding everything logged since construction.
//
// This turns a fallible, fully-journalled operation into a transaction: on any
// early return the destructor rolls back cleanly (while every mutated container
// is still alive, since the undo runs before those containers are torn down),
// and on success commit() folds the changes into the caller's frame so no extra
// frame lingers. The MHU uses it to add a head under always-on journaling.
template<typename IFrameControl>
struct trail_savepoint {
    explicit trail_savepoint(IFrameControl& frames);
    ~trail_savepoint();

    void commit();

    trail_savepoint(const trail_savepoint&) = delete;
    trail_savepoint& operator=(const trail_savepoint&) = delete;

private:
    IFrameControl& frames_;
    bool committed_;
};

template<typename IFrameControl>
trail_savepoint<IFrameControl>::trail_savepoint(IFrameControl& frames)
    : frames_(frames), committed_(false) {
    frames_.push();
}

template<typename IFrameControl>
trail_savepoint<IFrameControl>::~trail_savepoint() {
    if (!committed_)
        frames_.pop();
}

template<typename IFrameControl>
void trail_savepoint<IFrameControl>::commit() {
    frames_.squash_one();
    committed_ = true;
}

#endif
