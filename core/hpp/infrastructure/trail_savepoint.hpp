#ifndef TRAIL_SAVEPOINT_HPP
#define TRAIL_SAVEPOINT_HPP

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
