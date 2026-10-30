#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/global_functor_name_printer.hpp"
#include "infrastructure/id_var_printer.hpp"

using ::testing::ReturnRef;
using ::testing::StrictMock;

namespace {

struct MockGetFunctorName {
    MOCK_METHOD(const std::string&, functor_name, (uint32_t), (const));
};

struct GlobalFunctorNamePrinterTest : public ::testing::Test {
    StrictMock<MockGetFunctorName> names;
    global_functor_name_printer<StrictMock<MockGetFunctorName>> print_functor{names};
    id_var_printer print_var;
    std::ostringstream out;
    expr_printer<id_var_printer, global_functor_name_printer<StrictMock<MockGetFunctorName>>> printer{
        out, print_var, print_functor};
    expr_pool exprs;
    std::string eq{"eq"};
    std::string suc{"suc"};
    std::string zero{"zero"};
};

}

TEST_F(GlobalFunctorNamePrinterTest, PrintsASolutionWithFunctorNames) {
    EXPECT_CALL(names, functor_name(4u)).WillOnce(ReturnRef(eq));
    EXPECT_CALL(names, functor_name(5u)).WillOnce(ReturnRef(suc));
    EXPECT_CALL(names, functor_name(3u)).WillOnce(ReturnRef(zero));
    const expr* term = exprs.make_functor(4, {exprs.make_functor(5, {exprs.make_functor(3, {})})});
    printer.print(term);
    EXPECT_EQ(out.str(), "eq(suc(zero))");
}
