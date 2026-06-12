// print_progress: TTY-aware sim count rendering. Non-TTY path (captured stdout) wraps
// counts in newlines; finish_line is a no-op unless a TTY progress line is active.

#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/print_progress.hpp"

using ::testing::HasSubstr;
using ::testing::IsEmpty;

struct PrintProgressTest : public ::testing::Test {
    std::ostringstream captured;
    std::streambuf* old_out = nullptr;

    void SetUp() override {
        old_out = std::cout.rdbuf(captured.rdbuf());
    }

    void TearDown() override {
        std::cout.rdbuf(old_out);
    }
};

namespace {

constexpr size_t kCount1000 = 1000;
constexpr size_t kCount10000 = 10000;
constexpr size_t kCount999 = 999;

}  // namespace

TEST_F(PrintProgressTest, NonTtyPrintsNewlineWrappedCount) {
    print_progress progress;
    progress.print(kCount1000);

    EXPECT_THAT(captured.str(), HasSubstr("\n1000 sims\n"));
}

TEST_F(PrintProgressTest, FinishLineIsNoOpWhenNothingActive) {
    print_progress progress;
    progress.finish_line();

    EXPECT_THAT(captured.str(), IsEmpty());
}

TEST_F(PrintProgressTest, FinishLineAfterNonTtyPrintIsStillClean) {
    print_progress progress;
    progress.print(kCount1000);
    progress.finish_line();

    EXPECT_EQ(captured.str(), "\n1000 sims\n");
}

TEST_F(PrintProgressTest, SequentialPrintsEmitDistinctLines) {
    print_progress progress;
    progress.print(kCount10000);
    progress.print(kCount999);

    const std::string out = captured.str();
    EXPECT_THAT(out, HasSubstr("\n10000 sims\n"));
    EXPECT_THAT(out, HasSubstr("\n999 sims\n"));
}
