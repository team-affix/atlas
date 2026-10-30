#include <cstdio>
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include "infrastructure/hex_uint64.hpp"
#include "infrastructure/pud_module_create_command_handler.hpp"
#include "infrastructure/pud_module_inspect_command_handler.hpp"

namespace {

std::string temp_module_path() {
    static int counter = 0;
    return "/tmp/atlas_module_inspect_test_" + std::to_string(++counter) + ".pudm";
}

}

TEST(PudModuleInspectCommandHandlerTest, PrintsMetadataMatchingCreatedModule) {
    const std::string output_path = temp_module_path();
    std::ostringstream create_out;
    std::streambuf* old_out = std::cout.rdbuf(create_out.rdbuf());
    pud_module_create_command_handler create_handler{
        "cli/examples/member/db.chc",
        {},
        output_path};
    create_handler();
    std::cout.rdbuf(old_out);
    const std::string id_hex = create_out.str().substr(0, create_out.str().size() - 1);
    std::ostringstream inspect_out;
    old_out = std::cout.rdbuf(inspect_out.rdbuf());
    pud_module_inspect_command_handler inspect_handler{output_path, false, false, false};
    inspect_handler();
    std::cout.rdbuf(old_out);
    const std::string report = inspect_out.str();
    EXPECT_NE(report.find("module_id: " + id_hex), std::string::npos);
    EXPECT_NE(report.find("dependencies:\n"), std::string::npos);
    EXPECT_NE(report.find("axioms: "), std::string::npos);
    EXPECT_NE(report.find("inferences: 0\n"), std::string::npos);
    std::remove(output_path.c_str());
}

TEST(PudModuleInspectCommandHandlerTest, IdOnlyPrintsSingleHexLine) {
    const std::string output_path = temp_module_path();
    std::ostringstream create_out;
    std::streambuf* old_out = std::cout.rdbuf(create_out.rdbuf());
    pud_module_create_command_handler create_handler{
        "cli/examples/member/db.chc",
        {},
        output_path};
    create_handler();
    std::cout.rdbuf(old_out);
    const std::string expected_id = create_out.str();
    std::ostringstream inspect_out;
    old_out = std::cout.rdbuf(inspect_out.rdbuf());
    pud_module_inspect_command_handler inspect_handler{output_path, true, false, false};
    inspect_handler();
    std::cout.rdbuf(old_out);
    EXPECT_EQ(inspect_out.str(), expected_id);
    std::remove(output_path.c_str());
}

TEST(PudModuleInspectCommandHandlerTest, DepsOnlyPrintsSpaceSeparatedHexLine) {
    hex_uint64 hex_codec;
    const std::string output_path = temp_module_path();
    std::ostringstream create_out;
    std::streambuf* old_out = std::cout.rdbuf(create_out.rdbuf());
    pud_module_create_command_handler create_handler{
        "cli/examples/member/db.chc",
        {hex_codec.parse_hex("a1"), hex_codec.parse_hex("b2")},
        output_path};
    create_handler();
    std::cout.rdbuf(old_out);
    std::ostringstream inspect_out;
    old_out = std::cout.rdbuf(inspect_out.rdbuf());
    pud_module_inspect_command_handler inspect_handler{output_path, false, true, false};
    inspect_handler();
    std::cout.rdbuf(old_out);
    EXPECT_EQ(inspect_out.str(), "a1 b2\n");
    std::remove(output_path.c_str());
}

TEST(PudModuleInspectCommandHandlerTest, DetailedPrintsEachAxiomClause) {
    const std::string output_path = temp_module_path();
    std::ostringstream create_out;
    std::streambuf* old_out = std::cout.rdbuf(create_out.rdbuf());
    pud_module_create_command_handler create_handler{
        "cli/examples/member/db.chc",
        {},
        output_path};
    create_handler();
    std::cout.rdbuf(old_out);
    std::ostringstream inspect_out;
    old_out = std::cout.rdbuf(inspect_out.rdbuf());
    pud_module_inspect_command_handler inspect_handler{output_path, false, false, true};
    inspect_handler();
    std::cout.rdbuf(old_out);
    const std::string report = inspect_out.str();
    const std::string axioms_label = "axioms: ";
    const size_t axioms_at = report.find(axioms_label);
    ASSERT_NE(axioms_at, std::string::npos);
    const size_t axiom_count = static_cast<size_t>(std::stoul(report.substr(axioms_at + axioms_label.size())));
    EXPECT_GT(axiom_count, 0u);
    for (size_t node = 0; node < axiom_count; ++node) {
        const std::string line = "node " + std::to_string(node) + " axiom ";
        EXPECT_NE(report.find(line), std::string::npos) << line;
    }
    EXPECT_EQ(report.find("inference parent="), std::string::npos);
    std::remove(output_path.c_str());
}
