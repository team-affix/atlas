#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/hex_uint64.hpp"
#include "infrastructure/pud_module_create_command_handler.hpp"
#include "infrastructure/pud_module_deserializer_stack.hpp"

namespace {

std::string temp_module_path() {
    static int counter = 0;
    return "/tmp/atlas_module_test_" + std::to_string(++counter) + ".pudm";
}

}

TEST(PudModuleCreateCommandHandlerTest, WritesPudmAndPrintsHexModuleId) {
    hex_uint64 hex_codec;
    const std::string output_path = temp_module_path();
    std::ostringstream captured;
    std::streambuf* old_out = std::cout.rdbuf(captured.rdbuf());
    pud_module_create_command_handler handler{
        "cli/examples/member/db.chc",
        {hex_codec.parse_hex("a1"), hex_codec.parse_hex("b2")},
        output_path};
    handler();
    std::cout.rdbuf(old_out);
    const std::string printed_id = captured.str();
    ASSERT_FALSE(printed_id.empty());
    EXPECT_EQ(printed_id.back(), '\n');
    const std::string id_hex = printed_id.substr(0, printed_id.size() - 1);
    const uint64_t module_id = hex_codec.parse_hex(id_hex);
    std::ifstream in(output_path, std::ios::binary);
    ASSERT_TRUE(in);
    char magic[4] = {};
    in.read(magic, 4);
    EXPECT_EQ(magic[0], 'P');
    EXPECT_EQ(magic[1], 'U');
    EXPECT_EQ(magic[2], 'D');
    EXPECT_EQ(magic[3], 'M');
    in.seekg(0);
    pud_module_deserializer_stack deserializer_stack;
    const pud_module loaded = deserializer_stack.deserialize_module(in);
    EXPECT_EQ(loaded.get_module_id(), module_id);
    EXPECT_EQ(loaded.get_dependencies(), std::vector<uint64_t>({0xa1ull, 0xb2ull}));
    EXPECT_GT(loaded.count_axioms(), 0u);
    EXPECT_EQ(loaded.count_nodes(), loaded.count_axioms());
    std::remove(output_path.c_str());
}
