#include <cstdint>
#include <deque>
#include <filesystem>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_runtime.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_append_file.hpp"
#include "infrastructure/pud_mapped_record_file.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_module_pager.hpp"
#include "infrastructure/pud_module_serializer_stack.hpp"
#include "infrastructure/pud_paged_module_assembly.hpp"
#include "value_objects/pud_node_record.hpp"
#include "value_objects/rule.hpp"

// The paged module's files over a real scratch directory: records and payload survive reopening and growth, and a
// module paged and unpaged again serializes to exactly the bytes it started from.
struct PudPagedStorageIntegrationTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    std::filesystem::path scratch;

    void SetUp() override {
        scratch = std::filesystem::temp_directory_path()
                  / ("atlas_paged_storage_" + std::string(::testing::UnitTest::GetInstance()->current_test_info()->name()));
        std::filesystem::remove_all(scratch);
        std::filesystem::create_directories(scratch);
    }
    void TearDown() override { std::filesystem::remove_all(scratch); }

    const expr* var(uint32_t index) { return exprs.make_var(index); }
    const expr* atom(const char* name) { return exprs.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    static std::string serialized(const pud_module& module) {
        std::ostringstream out;
        pud_module_serializer_stack{}.serialize_module(out, module);
        return out.str();
    }

    // A Peano program grown by random unfolds through the manual runtime, its refutations included.
    pud_module grown_module(uint32_t seed, size_t unfolds) {
        std::deque<pud_module> modules;
        modules.emplace_back(31, std::vector<pud_module_id>{}, std::vector<rule>{
            rule(functor("nat", {atom("z")}), {}, 0),
            rule(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1),
            rule(functor("add", {atom("z"), var(0), var(0)}), {}, 1),
            rule(
                functor("add", {functor("s", {var(0)}), var(1), functor("s", {var(2)})}),
                {functor("add", {var(0), var(1), var(2)})},
                3),
            rule(functor("q", {var(0), var(1)}), {functor("nat", {var(0)}), functor("add", {var(0), var(0), var(1)})}, 2),
        });
        pud_runtime<pud_module, expr_pool> runtime(exprs);
        install_shared_functor_names(modules.front(), functors);
        std::set<pud_global_node_id> live;
        const pud_frontier_change registered = runtime.register_module(modules.front());
        live.insert(registered.offspring.begin(), registered.offspring.end());
        std::mt19937 rng(seed);
        for (size_t step = 0; step < unfolds; ++step) {
            std::vector<pud_global_node_id> unfoldable;
            for (const pud_global_node_id leaf : live)
                if (!runtime.get_rule_information(leaf).body.empty())
                    unfoldable.push_back(leaf);
            if (unfoldable.empty())
                break;
            const pud_global_node_id leaf = unfoldable.at(rng() % unfoldable.size());
            std::vector<pud_body_goal_id> goals;
            for (const auto& [goal, goal_expr] : runtime.get_rule_information(leaf).body)
                goals.push_back(goal);
            const pud_frontier_change change = runtime.unfold(leaf, goals.at(rng() % goals.size()));
            live.erase(leaf);
            live.insert(change.offspring.begin(), change.offspring.end());
            for (const pud_global_node_id refuted : change.refuted)
                live.erase(refuted);
        }
        return std::move(modules.front());
    }
};

TEST_F(PudPagedStorageIntegrationTest, RecordsSurviveGrowthAndReopening) {
    const std::string path = (scratch / "records").string();
    {
        pud_mapped_record_file records(path);
        for (uint64_t index = 0; index < 5000; ++index)
            records.append(pud_node_record{
                .payload_offset = index, .payload_size = 1, .parent = index / 2, .first_child = 0, .child_count = 0,
                .refuted = 0});
        records.set(4321, pud_node_record{
            .payload_offset = 9, .payload_size = 9, .parent = 9, .first_child = 9, .child_count = 9, .refuted = 1});
        records.sync();
    }
    pud_mapped_record_file reopened(path);
    ASSERT_EQ(reopened.count(), 5000u);
    EXPECT_EQ(reopened.get(4999).parent, 2499u);
    EXPECT_EQ(reopened.get(4321).refuted, 1u);
    EXPECT_EQ(reopened.get(0).payload_offset, 0u);
}

TEST_F(PudPagedStorageIntegrationTest, AppendedBytesAreReadBackAtTheirOffsetsAfterReopening) {
    const std::string path = (scratch / "payload").string();
    uint64_t second = 0;
    {
        pud_append_file payload(path);
        EXPECT_EQ(payload.append("first"), 0u);
        second = payload.append("second");
        payload.sync();
    }
    pud_append_file reopened(path);
    EXPECT_EQ(reopened.size(), 11u);
    EXPECT_EQ(reopened.read(second, 6), "second");
    EXPECT_EQ(reopened.append("third"), 11u);
    EXPECT_EQ(reopened.read(0, 5), "first");
}

TEST_F(PudPagedStorageIntegrationTest, AModulePagedAndUnpagedSerializesToTheSameBytes) {
    const pud_module grown = grown_module(5, 80);
    ASSERT_GT(grown.count_nodes(), 30u);
    const std::string directory = (scratch / "module").string();
    pud_module_pager{}.page(grown, directory, exprs);
    pud_paged_module_assembly assembly(directory, exprs, 0);
    EXPECT_EQ(serialized(pud_module_pager{}.unpage(assembly.module())), serialized(grown));
}

TEST_F(PudPagedStorageIntegrationTest, AReopenedModuleHoldsWhatWasStoredBeforeIt) {
    const pud_module grown = grown_module(9, 40);
    const std::string directory = (scratch / "module").string();
    pud_module_pager{}.page(grown, directory, exprs);
    pud_node_id added = 0;
    pud_inference copied{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0};
    {
        pud_paged_module_assembly assembly(directory, exprs, 0);
        pud_paged_module_assembly::module_t& module = assembly.module();
        pud_node_id leaf = 0;
        while (!module.is_leaf(leaf) || module.is_refuted(leaf))
            ++leaf;
        copied = module.get_inference(module.count_nodes() - 1);
        copied.parent = leaf;
        added = module.store_inference(copied);
        module.mark_refuted(added);
        module.append_functor_name("fresh");
        assembly.sync();
    }
    pud_paged_module_assembly reopened(directory, exprs, 0);
    pud_paged_module_assembly::module_t& module = reopened.module();
    EXPECT_EQ(module.count_nodes(), grown.count_nodes() + 1);
    EXPECT_EQ(module.get_inference(added), copied);
    EXPECT_TRUE(module.is_refuted(added));
    EXPECT_EQ(module.get_parent(added), copied.parent);
    EXPECT_EQ(module.functor_names().back(), "fresh");
}
