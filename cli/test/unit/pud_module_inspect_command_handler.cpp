#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include "infrastructure/hex_uint64.hpp"
#include "infrastructure/pud_module_create_command_handler.hpp"
#include "infrastructure/pud_module_inspect_command_handler.hpp"
#include "infrastructure/pud_module_deserializer_stack.hpp"
#include "infrastructure/pud_module_serializer_stack.hpp"
#include "value_objects/pud_inference.hpp"

namespace {

// Creates the member example module, appends one inference under axiom 0 and writes it back to path.
void create_module_with_inference(const std::string& path) {
    std::ostringstream create_out;
    std::streambuf* old_out = std::cout.rdbuf(create_out.rdbuf());
    pud_module_create_command_handler create_handler{"cli/examples/member/db.chc", {}, path};
    create_handler();
    std::cout.rdbuf(old_out);
    std::ifstream in(path, std::ios::binary);
    pud_module_deserializer_stack deserializer_stack;
    pud_module module = deserializer_stack.deserialize_module(in);
    in.close();
    module.store_inference(pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0});
    std::ofstream out(path, std::ios::binary);
    pud_module_serializer_stack serializer_stack;
    serializer_stack.serialize_module(out, module);
}

std::string inspect_report(const std::string& path, bool show_inferences) {
    std::ostringstream inspect_out;
    std::streambuf* old_out = std::cout.rdbuf(inspect_out.rdbuf());
    pud_module_inspect_command_handler inspect_handler{path, false, false, true, show_inferences};
    inspect_handler();
    std::cout.rdbuf(old_out);
    return inspect_out.str();
}

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
    pud_module_inspect_command_handler inspect_handler{output_path, false, false, false, false};
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
    pud_module_inspect_command_handler inspect_handler{output_path, true, false, false, false};
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
    pud_module_inspect_command_handler inspect_handler{output_path, false, true, false, false};
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
    pud_module_inspect_command_handler inspect_handler{output_path, false, false, true, false};
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

TEST(PudModuleInspectCommandHandlerTest, DetailedLeavesOutInferencesButStillCountsThem) {
    const std::string output_path = temp_module_path();
    create_module_with_inference(output_path);
    const std::string report = inspect_report(output_path, false);
    EXPECT_NE(report.find("inferences: 1\n"), std::string::npos);
    EXPECT_NE(report.find("node 0 axiom "), std::string::npos);
    EXPECT_EQ(report.find("inference parent="), std::string::npos);
    std::remove(output_path.c_str());
}

TEST(PudModuleInspectCommandHandlerTest, ShowInferencesAlsoPrintsEachInferenceNode) {
    const std::string output_path = temp_module_path();
    create_module_with_inference(output_path);
    const std::string report = inspect_report(output_path, true);
    EXPECT_NE(report.find("node 0 axiom "), std::string::npos);
    EXPECT_NE(report.find(" inference parent=0 call_site=0\n"), std::string::npos);
    std::remove(output_path.c_str());
}
