// unfold_command_handler: dual-file import, one-step unfold; default prints
// original + unfolded rules, --print-db prints the mutated derivation DB,
// --overwrite writes that DB back to the derivation file.

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/unfold_command_handler.hpp"

using ::testing::HasSubstr;
using ::testing::Not;

namespace {

constexpr const char* kAncestorDb = "cli/examples/ancestor/db.chc";
constexpr rule_id kUnitAncestorRuleId = 5;
constexpr subgoal_id kFirstBodyIdx = 0;
constexpr bool kPrintDiff = false;
constexpr bool kPrintDb = true;
constexpr bool kOverwrite = true;
constexpr bool kKeepFile = false;

std::string run_unfold_capture(
    const std::string& basis,
    const std::string& derivation,
    rule_id subject_id,
    subgoal_id body_idx,
    bool print_db,
    bool overwrite) {
    std::ostringstream captured;
    std::streambuf* old_out = std::cout.rdbuf(captured.rdbuf());
    unfold_command_handler(basis, derivation, subject_id, body_idx, print_db, overwrite)();
    std::cout.rdbuf(old_out);
    return captured.str();
}

std::string read_file(const std::string& path) {
    std::ifstream in(path);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

}  // namespace

struct UnfoldCommandHandlerTest : public ::testing::Test {};

TEST_F(UnfoldCommandHandlerTest, ConstructsFromAncestorExample) {
    EXPECT_NO_THROW(unfold_command_handler(
        kAncestorDb, kAncestorDb, kUnitAncestorRuleId, kFirstBodyIdx, kPrintDiff, kKeepFile));
}

TEST_F(UnfoldCommandHandlerTest, MissingBasisFileThrows) {
    EXPECT_THROW(
        unfold_command_handler(
            "cli/examples/nonexistent/db.chc", kAncestorDb,
            kUnitAncestorRuleId, kFirstBodyIdx, kPrintDiff, kKeepFile),
        std::runtime_error);
}

TEST_F(UnfoldCommandHandlerTest, MissingDerivationFileThrows) {
    EXPECT_THROW(
        unfold_command_handler(
            kAncestorDb, "cli/examples/nonexistent/db.chc",
            kUnitAncestorRuleId, kFirstBodyIdx, kPrintDiff, kKeepFile),
        std::runtime_error);
}

TEST_F(UnfoldCommandHandlerTest, PrintDbAndOverwriteTogetherThrows) {
    EXPECT_THROW(
        unfold_command_handler(
            kAncestorDb, kAncestorDb, kUnitAncestorRuleId, kFirstBodyIdx, kPrintDb, kOverwrite),
        std::runtime_error);
}

TEST_F(UnfoldCommandHandlerTest, DefaultPrintsOriginalThenUnfoldedRules) {
    const std::string out = run_unfold_capture(
        kAncestorDb, kAncestorDb, kUnitAncestorRuleId, kFirstBodyIdx, kPrintDiff, kKeepFile);

    EXPECT_THAT(out, HasSubstr("ancestor(?0, ?1) :- parent(?0, ?1)."));
    EXPECT_THAT(out, HasSubstr("---"));
    EXPECT_THAT(out, HasSubstr("ancestor(tom, bob)."));
    EXPECT_THAT(out, HasSubstr("ancestor(tom, liz)."));
    EXPECT_THAT(out, HasSubstr("ancestor(bob, ann)."));
    EXPECT_THAT(out, HasSubstr("ancestor(bob, pat)."));
    EXPECT_THAT(out, HasSubstr("ancestor(pat, jim)."));
    EXPECT_THAT(out, Not(HasSubstr("parent(tom, bob).")));
    EXPECT_THAT(out, Not(HasSubstr("ancestor(?0, ?1) :- parent(?0, ?2), ancestor(?2, ?1).")));
    EXPECT_LT(out.find("ancestor(?0, ?1) :- parent(?0, ?1)."), out.find("---"));
    EXPECT_LT(out.find("---"), out.find("ancestor(tom, bob)."));
}

TEST_F(UnfoldCommandHandlerTest, PrintDbWritesMutatedDerivationDb) {
    const std::string out = run_unfold_capture(
        kAncestorDb, kAncestorDb, kUnitAncestorRuleId, kFirstBodyIdx, kPrintDb, kKeepFile);

    EXPECT_THAT(out, HasSubstr("parent(tom, bob)."));
    EXPECT_THAT(out, HasSubstr("parent(tom, liz)."));
    EXPECT_THAT(out, HasSubstr("parent(bob, ann)."));
    EXPECT_THAT(out, HasSubstr("parent(bob, pat)."));
    EXPECT_THAT(out, HasSubstr("parent(pat, jim)."));
    EXPECT_THAT(out, HasSubstr("ancestor(_0, _1) :- parent(_0, _2), ancestor(_2, _1)."));
    EXPECT_THAT(out, Not(HasSubstr("ancestor(?0, ?1) :- parent(?0, ?1).")));
    EXPECT_THAT(out, HasSubstr("ancestor(tom, bob)."));
    EXPECT_THAT(out, HasSubstr("ancestor(tom, liz)."));
    EXPECT_THAT(out, HasSubstr("ancestor(bob, ann)."));
    EXPECT_THAT(out, HasSubstr("ancestor(bob, pat)."));
    EXPECT_THAT(out, HasSubstr("ancestor(pat, jim)."));
    EXPECT_THAT(out, Not(HasSubstr("---")));
}

TEST_F(UnfoldCommandHandlerTest, OverwriteWritesMutatedDbToDerivationFile) {
    const std::string expected = run_unfold_capture(
        kAncestorDb, kAncestorDb, kUnitAncestorRuleId, kFirstBodyIdx, kPrintDb, kKeepFile);

    const std::filesystem::path tmp =
        std::filesystem::temp_directory_path() / "atlas_unfold_overwrite.chc";
    std::filesystem::copy_file(
        kAncestorDb, tmp, std::filesystem::copy_options::overwrite_existing);

    const std::string out = run_unfold_capture(
        kAncestorDb, tmp.string(), kUnitAncestorRuleId, kFirstBodyIdx, kPrintDiff, kOverwrite);

    EXPECT_EQ(out, "");
    EXPECT_EQ(read_file(tmp.string()), expected);
    std::filesystem::remove(tmp);
}
