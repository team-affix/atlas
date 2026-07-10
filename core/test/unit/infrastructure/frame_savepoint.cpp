// frame_savepoint: RAII transaction over push_frame / pop_frame / squash_frame.

#include <gtest/gtest.h>
#include "infrastructure/frame_savepoint.hpp"

namespace {

struct recording_frame_control {
    int push_count = 0;
    int pop_count = 0;
    int squash_count = 0;

    void push_frame() { ++push_count; }
    void pop_frame() { ++pop_count; }
    void squash_frame() { ++squash_count; }
};

TEST(FrameSavepointTest, DestructorPopsWhenNotCommitted) {
    recording_frame_control fc;
    {
        frame_savepoint sp{fc};
        EXPECT_EQ(fc.push_count, 1);
        EXPECT_EQ(fc.pop_count, 0);
    }
    EXPECT_EQ(fc.pop_count, 1);
    EXPECT_EQ(fc.squash_count, 0);
}

TEST(FrameSavepointTest, CommitSquashesInsteadOfPopping) {
    recording_frame_control fc;
    {
        frame_savepoint sp{fc};
        sp.commit();
        EXPECT_EQ(fc.push_count, 1);
    }
    EXPECT_EQ(fc.pop_count, 0);
    EXPECT_EQ(fc.squash_count, 1);
}

}  // namespace
