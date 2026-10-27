#include <cstdio>
#include <sstream>
#include <string>
#include <gtest/gtest.h>
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
    pud_module_inspect_command_handler inspect_handler{output_path};
    inspect_handler();
    std::cout.rdbuf(old_out);
    const std::string report = inspect_out.str();
    EXPECT_NE(report.find("module_id: " + id_hex), std::string::npos);
    EXPECT_NE(report.find("dependencies:\n"), std::string::npos);
    EXPECT_NE(report.find("axioms: "), std::string::npos);
    EXPECT_NE(report.find("inferences: 0\n"), std::string::npos);
    std::remove(output_path.c_str());
}
