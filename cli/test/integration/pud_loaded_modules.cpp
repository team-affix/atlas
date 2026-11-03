#include <cstdio>
#include <deque>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/hex_uint64.hpp"
#include "infrastructure/pud_loaded_modules.hpp"
#include "infrastructure/pud_module_create_command_handler.hpp"

namespace {

std::string temp_module_path(const char* label) {
    static int counter = 0;
    return std::string("/tmp/atlas_loaded_modules_") + label + "_" + std::to_string(++counter) + ".pudm";
}

std::string create_module(const std::vector<pud_module_id>& dependencies) {
    const std::string output_path = temp_module_path("mod");
    std::ostringstream discarded;
    std::streambuf* old_out = std::cout.rdbuf(discarded.rdbuf());
    pud_module_create_command_handler create_handler{
        "cli/examples/member/db.chc",
        dependencies,
        output_path};
    create_handler();
    std::cout.rdbuf(old_out);
    const std::string printed = discarded.str();
    return output_path;
}

pud_module_id created_id(const std::string& path) {
    pud_loaded_modules loaded({path});
    return loaded.modules().front().get_module_id();
}

}

TEST(PudLoadedModulesTest, LoadsASingleModule) {
    const std::string path = create_module({});
    pud_loaded_modules loaded({path});
    EXPECT_EQ(loaded.modules().size(), 1u);
    std::remove(path.c_str());
}

TEST(PudLoadedModulesTest, RegistersDependenciesBeforeDependents) {
    const std::string dependency_path = create_module({});
    const pud_module_id dependency_id = created_id(dependency_path);
    const std::string entry_path = create_module({dependency_id});
    pud_loaded_modules loaded({entry_path, dependency_path});
    ASSERT_EQ(loaded.modules().size(), 2u);
    EXPECT_EQ(loaded.modules().front().get_module_id(), dependency_id);
    std::remove(dependency_path.c_str());
    std::remove(entry_path.c_str());
}

TEST(PudLoadedModulesTest, RejectsAMissingDependency) {
    hex_uint64 hex_codec;
    const std::string path = create_module({hex_codec.parse_hex("abc")});
    EXPECT_THROW(pud_loaded_modules loaded({path}), std::runtime_error);
    std::remove(path.c_str());
}

TEST(PudLoadedModulesTest, SavingWritesEachModuleBackToItsOwnFileWithWhatTheSessionAdded) {
    const std::string dependency_path = create_module({});
    const pud_module_id dependency_id = created_id(dependency_path);
    const std::string entry_path = create_module({dependency_id});
    size_t entry_nodes = 0;
    {
        pud_loaded_modules loaded({entry_path, dependency_path});
        pud_module& entry = loaded.modules().back();
        entry.store_inference(pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0});
        entry.mark_refuted(0);
        entry_nodes = entry.count_nodes();
        loaded.save_all();
    }
    EXPECT_FALSE(std::filesystem::exists(entry_path + ".tmp"));
    EXPECT_FALSE(std::filesystem::exists(dependency_path + ".tmp"));
    pud_loaded_modules reloaded({entry_path, dependency_path});
    ASSERT_EQ(reloaded.modules().size(), 2u);
    EXPECT_EQ(reloaded.modules().front().get_module_id(), dependency_id);
    const pud_module& entry = reloaded.modules().back();
    EXPECT_EQ(entry.count_nodes(), entry_nodes);
    EXPECT_TRUE(entry.is_refuted(0));
    EXPECT_EQ(created_id(dependency_path), dependency_id);
    std::remove(dependency_path.c_str());
    std::remove(entry_path.c_str());
}

TEST(PudLoadedModulesTest, SavingUnchangedModulesLeavesThemLoadable) {
    const std::string path = create_module({});
    const pud_module_id id = created_id(path);
    pud_loaded_modules({path}).save_all();
    EXPECT_EQ(created_id(path), id);
    std::remove(path.c_str());
}
