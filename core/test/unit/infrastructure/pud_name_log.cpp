#include <cstdint>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_name_log.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;

namespace {

struct MockAppendBytes {
    MOCK_METHOD(uint64_t, append, (const std::string&));
};
struct MockReadBytes {
    MOCK_METHOD(std::string, read, (uint64_t, uint64_t), (const));
    MOCK_METHOD(uint64_t, size, (), (const));
};

std::string length_prefixed(const std::string& name) {
    std::string entry(8, '\0');
    entry.at(0) = static_cast<char>(name.size());
    return entry + name;
}

struct PudNameLogTest : public ::testing::Test {
    NiceMock<MockAppendBytes> appender;
    NiceMock<MockReadBytes> reader;
    pud_name_log<NiceMock<MockAppendBytes>, NiceMock<MockReadBytes>> log{appender, reader};
};

}

TEST_F(PudNameLogTest, ANameIsAppendedWithItsLengthBeforeIt) {
    EXPECT_CALL(appender, append(length_prefixed("succ")));
    log.append("succ");
}

TEST_F(PudNameLogTest, NamesAreReadBackInTheOrderTheyWereAppended) {
    const std::string bytes = length_prefixed("zero") + length_prefixed("") + length_prefixed("succ");
    ON_CALL(reader, size()).WillByDefault(Return(bytes.size()));
    EXPECT_CALL(reader, read(0u, bytes.size())).WillOnce(Return(bytes));
    EXPECT_THAT(log.read_all(), ElementsAre("zero", "", "succ"));
}

TEST_F(PudNameLogTest, AnEmptyLogHasNoNames) {
    ON_CALL(reader, size()).WillByDefault(Return(0u));
    ON_CALL(reader, read(_, _)).WillByDefault(Return(std::string{}));
    EXPECT_THAT(log.read_all(), IsEmpty());
}
