#include <cstdint>
#include <optional>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_functor_relinker.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetGlobalFunctorId {
    MOCK_METHOD(uint32_t, get_global_functor_id_from_local_functor_id, (pud_module_id, uint32_t));
};
struct MockFindLocalFunctorId {
    MOCK_METHOD(std::optional<uint32_t>, find_local_functor_id, (pud_module_id, uint32_t));
};
struct MockGetNameByGlobalFunctorId {
    MOCK_METHOD(const std::string&, get_name_by_global_functor_id, (uint32_t));
};
struct MockAppendFunctorName {
    MOCK_METHOD(uint32_t, append_functor_name, (pud_module_id, const std::string&));
};
struct MockInsertLink {
    MOCK_METHOD(void, insert_link, (pud_module_id, uint32_t, uint32_t));
};

using test_relinker_t = pud_functor_relinker<
    MockGetGlobalFunctorId, MockFindLocalFunctorId, MockGetNameByGlobalFunctorId, MockAppendFunctorName,
    MockInsertLink>;

struct PudFunctorRelinkerTest : public ::testing::Test {
    MockGetGlobalFunctorId global_ids;
    MockFindLocalFunctorId local_ids;
    MockGetNameByGlobalFunctorId names;
    MockAppendFunctorName append;
    MockInsertLink links;
    test_relinker_t relinker{global_ids, local_ids, names, append, links};
    std::string succ{"succ"};
};

}

TEST_F(PudFunctorRelinkerTest, AFunctorTheTargetModuleAlreadyLinksKeepsItsLocalId) {
    InSequence order;
    EXPECT_CALL(global_ids, get_global_functor_id_from_local_functor_id(3u, 6u)).WillOnce(Return(60u));
    EXPECT_CALL(local_ids, find_local_functor_id(8u, 60u)).WillOnce(Return(16u));
    EXPECT_CALL(names, get_name_by_global_functor_id(_)).Times(0);
    EXPECT_CALL(append, append_functor_name(_, _)).Times(0);
    EXPECT_CALL(links, insert_link(_, _, _)).Times(0);
    EXPECT_EQ(relinker.relink(3, 6, 8), 16u);
}

TEST_F(PudFunctorRelinkerTest, AFunctorTheTargetModuleLacksIsAppendedByNameAndLinked) {
    InSequence order;
    EXPECT_CALL(global_ids, get_global_functor_id_from_local_functor_id(3u, 6u)).WillOnce(Return(60u));
    EXPECT_CALL(local_ids, find_local_functor_id(8u, 60u)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(names, get_name_by_global_functor_id(60u)).WillOnce(ReturnRef(succ));
    EXPECT_CALL(append, append_functor_name(8u, succ)).WillOnce(Return(4u));
    EXPECT_CALL(links, insert_link(8u, 4u, 60u));
    EXPECT_EQ(relinker.relink(3, 6, 8), 4u);
}
