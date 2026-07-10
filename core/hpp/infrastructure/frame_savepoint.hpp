#ifndef FRAME_SAVEPOINT_HPP
#define FRAME_SAVEPOINT_HPP

template<typename IFrameControl>
struct frame_savepoint {
    explicit frame_savepoint(IFrameControl& frames);
    ~frame_savepoint();

    void commit();

    frame_savepoint(const frame_savepoint&) = delete;
    frame_savepoint& operator=(const frame_savepoint&) = delete;

private:
    IFrameControl& frames_;
    bool committed_;
};

template<typename IFrameControl>
frame_savepoint<IFrameControl>::frame_savepoint(IFrameControl& frames)
    : frames_(frames), committed_(false) {
    frames_.push_frame();
}

template<typename IFrameControl>
frame_savepoint<IFrameControl>::~frame_savepoint() {
    if (!committed_)
        frames_.pop_frame();
}

template<typename IFrameControl>
void frame_savepoint<IFrameControl>::commit() {
    frames_.squash_frame();
    committed_ = true;
}

#endif
